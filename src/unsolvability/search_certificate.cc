#include "search_certificate.h"
#include "simple_certificate.h"

#include <cassert>
#include <fstream>
#include <sstream>

SearchCertificate::SearchCertificate(Task *task, std::ifstream &in)
     : Certificate(task) {

    print_info("reading in search certificate");

    std::string line;
    std::getline(in, line);
    std::vector<std::string> line_arr;
    split(line, line_arr, ':');
    if(line_arr[0].compare("File") != 0) {
        print_parsing_error_and_exit(line, "File:<filename");
    }
    assert(line_arr.size() == 2);
    std::string certificate_file = line_arr[1];
    std::cout << "search certificate file: " << certificate_file << std::endl;
    std::getline(in, line);
    if(line.compare("begin_variables") != 0) {
        print_parsing_error_and_exit(line, "begin_variables");
    }
    read_in_variable_order(in);
    std::vector<BDD> bddvec;
    parse_bdd_file(certificate_file, bddvec);
    assert(bddvec.size() == 2);
    bdd_exp = bddvec[0];
    bdd_pr = bddvec[1];

    //create a mapping from bdd vars back to the global facts
    bddvar_to_global_fact = std::vector<int>(task->get_number_of_facts()*2, -1);
    for(size_t i = 0; i < task->get_number_of_facts(); ++i) {
        bddvar_to_global_fact[fact_to_bddvar[i]] = i;
    }

    //parse subcertificates
    std::getline(in, line);
    split(line, line_arr, ':');
    if(line_arr[0].compare("subcertificates") != 0) {
        print_parsing_error_and_exit(line, "subcertificates:<#subcertificates");
    }
    int subcertificates_amount = stoi(line_arr[1]);
    std::cout << "parsing " << subcertificates_amount << " subcertificates" << std::endl;
    h_certificates.resize(subcertificates_amount);
    for(int i = 0; i < subcertificates_amount; ++i) {
        std::getline(in, line);

        if(line.compare("simple_certificate") == 0) {
            std::cout << "subcertificate " << i << ": Simple certificate" << std::endl;
            h_certificates[i]= new SimpleCertificate(task, in);
        } else if(line.compare("strong_conjunctive_certificate") == 0) {
            //TODO
            std::cout << "not yet implemented (strong conjunctive certificate)" << std::endl;
            exit(0);
        } else {
            std::cout << "subcertificate " << i
                      << " type unknown: " << line << std::endl;
            exit(0);
        }
    }
    std::getline(in, line);
    if(line.compare("end_subcertificates") != 0) {
        print_parsing_error_and_exit(line, "end_subcertificats");
    }
    std::getline(in, line);
    if(line.compare("end_certificate") != 0) {
        print_parsing_error_and_exit(line, "end_certificate");
    }
    print_info("finished reading in search certificate");
}

/* The certificate contains a cube if it is either in the BDD representing
   the expanded states or in one of the h_certificates */

bool SearchCertificate::contains_cube(const Cube &cube) {
    //TODO: IndicesToCube?
    BDD statebdd = manager.bddOne();
    for(size_t i = 0; i < cube.size(); ++i) {
      if(cube[i] == 1) {
        statebdd = statebdd * manager.bddVar(fact_to_bddvar[i]);
      } else if(cube[i] == 0){
        statebdd = statebdd - manager.bddVar(fact_to_bddvar[i]);
      } else {
          // cube[i] == 2 means we don't care about the truth assignment
          assert(cube[i] == 2);
      }
    }
    //TODO: leq?
    BDD result = bdd_exp * statebdd;
    if(!result.IsZero()) {
      return true;
    }
    for(size_t i = 0; i < h_certificates.size(); ++i) {
        if(h_certificates[i]->contains_cube(cube)) {
            return true;
        }
    }
    return false;
}

bool SearchCertificate::is_inductive() {
    // FIRST PART: check if all successors are contained in bdd_exp \union bdd_pr

    // permutation for renaming the certificate to the primed variables
    int factamount = task->get_number_of_facts();
    int permutation[factamount*2];
    for(int i = 0 ; i < factamount; ++i) {
      permutation[2*i] = (2*i)+1;
      permutation[(2*i)+1] = 2*i;
    }
    BDD exp_pr_union = bdd_exp + bdd_pr;

    BDD union_primed = exp_pr_union.Permute(permutation);

    print_info("checking if actions are inductive");

    // loop over all actions
    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        // info output
        if(i%100 == 0) {
            std::stringstream tmp;
            tmp << "action " << i;
            print_info(tmp.str());
        }
        BDD action_bdd = build_bdd_for_action(task->get_action(i));

        // succ represents pairs of states and successors with action a
        BDD succ = action_bdd * bdd_exp;
        //TODO: leq?
        BDD res = succ - union_primed;
        // res contains all pairs of states where the successor is in neither c nor bdd_pr
        // if it is not empty, then a successor from a state of c
        // is not covered anywhere and thus the certificate is not inductive
        if(!res.IsZero()) {
            std::cout << "action " << i << " is not inductive" << std::endl;
            return false;
        }
    } // end loop over actions


    // if there are no pruned states, bbd_exp is inductive already --> we are done
    if(bdd_pr.IsZero()) {
        print_info("bdd_pr is empty - no further checking needed");
        return true;
    }


    // SECOND PART: see if all states in bdd_pr are covered by h_certificates
    print_info("checking if pruned states are contained in h certificate");
    int* cube;
    CUDD_VALUE_TYPE value_type;
    DdGen * cubegen = bdd_pr.FirstCube(&cube, &value_type);
    bool done = false;
    int count = 0; // only used for info output

    // loop over all states in bdd_pr and see if they are in any h_certificate
    while(!done) {
        // info output
        if(count%10000 == 0) {
            std::stringstream tmp;
            tmp << "state " << count;
            print_info(tmp.str());
        }
        // this cube works with global variable ordering --> use bddvar_to_global_fact
        Cube s = std::vector<int>(factamount, 0);
        /*
         Don't cares are variables which can be either true or false
         We need to test each state (as it occured in the original search) separately, since
         it can be that two pruned states only differ in one variable but landed in different
         h certificates. Thus we need to test for each don't care (which represents a variable
         used in the search) both versions separately.
         */
        std::vector<int> dont_cares;
        // we are only interested in the unprimed (=even) variables
        for(int i = 0; i < factamount; ++i) {
            if(cube[2*i] == 1) {
                s[bddvar_to_global_fact[2*i]] = 1;
            } else if(cube[2*i] == 2) {
                // if the don't care variable did not occur in the original search
                // (-> index >= original_bdd_vars) then we do not need to distinguish this
                // variable because it would not result in two distinct search states
                if(i < original_bdd_vars) {
                    dont_cares.push_back(bddvar_to_global_fact[2*i]);
                }
                s[bddvar_to_global_fact[2*i]] = 2;
            } else {
                assert(cube[2*i] == 0);
            }
        }

        int combinations = 1 << dont_cares.size();

        // try out all combinations of true/false for the don't cares
        for(int i = 0; i < combinations; ++i) {
            // i acts as a binary encoding which vars should be true/false
            // Example: i = 13, #don't cares = 4
            // i = 1101 --> first, third and fourth don't care true, second false
            // (read right to left, it's less to calculate this way)
            for(size_t j = 0; j < dont_cares.size(); ++j) {
                if( ((i >> j) % 2) == 0 ) {
                    s[dont_cares[j]] = 0;
                } else {
                    s[dont_cares[j]] = 1;
                }
            }
            // see if the state is contained in any of the h certificates
            if(!is_in_h_certificates(s)) {
                std::cout << "state ";
                for(size_t i = 0; i < s.size(); ++i) {
                    std::cout << (int)s[i];
                }
                std::cout << " is not in h certificate" << std::endl;
                return false;
            }
            count++;
        }

        if(Cudd_NextCube(cubegen, &cube, &value_type) == 0) {
            done = true;
        }
    }
    // TODO: only check inductivity for those h_certificates which were actually used
    // (and inform if some were not used)

    // THIRD PART: see if all h_certificates are inductive
    for(size_t i = 0; i < h_certificates.size(); ++i) {
        // info output
        std::stringstream tmp;
        tmp << "checking if h certificate " << i << " is inductive";
        print_info(tmp.str());

        if(!h_certificates[i]->is_inductive()) {
            std::cout << "h certificate not inductive!" << std::endl;
            return false;
        }
    }

    // all test passed, the certificate is valid
    return true;
}

bool SearchCertificate::is_in_h_certificates(Cube& s) {
    for(size_t i = 0; i < h_certificates.size(); ++i) {
        if(h_certificates[i]->contains_cube(s)) {
            return true;
        }
    }
    return false;
}
