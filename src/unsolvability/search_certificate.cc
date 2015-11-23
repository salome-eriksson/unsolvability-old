#include "search_certificate.h"
#include "simple_certificate.h"

#include "math.h"
#include <cassert>
#include <fstream>
#include <sstream>

SearchCertificate::SearchCertificate(Task *task, std::ifstream &in)
     : Certificate(task) {

    print_info("reading in search certificate");

    manager = Cudd(task->get_number_of_facts()*2,0);

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
    map_global_facts_to_bdd_var(in);
    std::vector<BDD> bddvec;
    parse_bdd_file(certificate_file, bddvec);
    assert(bddvec.size() == 2);
    bdd_exp = bddvec[0];
    bdd_pr = bddvec[1];

    //create a mapping from bdd vars back to the global facts
    bdd_to_global_var_mapping = std::vector<int>(task->get_number_of_facts()*2, -1);
    for(size_t i = 0; i < task->get_number_of_facts(); ++i) {
        bdd_to_global_var_mapping[fact_to_bddvar[i]] = i;
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
    BDD statebdd = manager.bddOne();
    for(size_t i = 0; i < cube.size(); ++i) {
      if(cube[i] == 1) {
        statebdd = statebdd * manager.bddVar(fact_to_bddvar[i]);
      } else if(cube[i] == 0){
        statebdd = statebdd - manager.bddVar(fact_to_bddvar[i]);
      } else {
          //if cube[i] == 2 this means we don't care about the truth assignment
          assert(cube[i] == 2);
      }
    }
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
    //first part: check if all successors are contained in bdd_exp \union bdd_pr

    //permutation for renaming the certificate to the primed variables
    int factamount = task->get_number_of_facts();
    int permutation[factamount*2];
    for(int i = 0 ; i < factamount; ++i) {
      permutation[2*i] = (2*i)+1;
      permutation[(2*i)+1] = 2*i;
    }
    BDD exp_pr_union = bdd_exp + bdd_pr;

    BDD union_primed = exp_pr_union.Permute(permutation);

    print_info("checking if actions are inductive");

    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        if(i%100 == 0) {
            std::stringstream tmp;
            tmp << "action " << i;
            print_info(tmp.str());
        }
        const Action &a = task->get_action(i);
        BDD action_bdd = build_bdd_for_action(a);

        // succ represents pairs of states and successors with action a
        BDD succ = action_bdd * bdd_exp;
        BDD res = succ - union_primed;
        // res contains all pairs of states where the successor is in neither c nor bdd_pr
        // if it is not empty, then a successor from a state of c
        // is not covered anywhere and thus the certificate is not inductive
        if(!res.IsZero()) {
            std::cout << "action " << i << " is not inductive" << std::endl;
            return false;
        }
    } //end loop over actions


    //if there are no pruned states, bbd_exp is inductive already --> we are done
    if(bdd_pr.IsZero()) {
        return true;
    }


    //second part: see if all states in bdd_pr are covered by h_certificates
    //and all h_certificates are inductive

    print_info("checking if pruned states are contained in h certificate");
    //the bdd contains all facts from the task, plus their primed version
    int* cube;
    CUDD_VALUE_TYPE value_type;
    DdGen * cubegen = bdd_pr.FirstCube(&cube, &value_type);
    bool done = false;
    int count = 0;

    //loop over all states in bdd_pr and see if they are in any h_certificate
    while(!done) {
        if(count%10000 == 0) {
            std::stringstream tmp;
            tmp << "state " << count;
            print_info(tmp.str());
        }
        Cube s = std::vector<int>(factamount, 0);
        std::vector<int> undef_vars;
        //the 2*i is because we are only interested in unprimed (=even) variables
        //(the uneven variables are all primed)
        for(int i = 0; i < factamount; ++i) {
            if(cube[2*i] == 1) {
                s[bdd_to_global_var_mapping[2*i]] = 1;
            //All variables which have a higher or equal bdd index than #original bdd vars (*2)
            //did not occur in the search and thus it cannot happen that two states depending
            //on only this variable would end up in two different h certificates
            } else if(cube[2*i] == 2) {
                if(i < original_bdd_vars) {
                    undef_vars.push_back(bdd_to_global_var_mapping[2*i]);
                }
                s[bdd_to_global_var_mapping[2*i]] = 2;
            } else {
                assert(cube[2*i] == 0);
            }
        }

        //try out all combinations of true/false for the undefined variables
        //TODO: test this!
        for(int i = 0; i < pow(2,undef_vars.size()); ++i) {
            for(size_t j = 0; j < undef_vars.size(); ++j) {
                if(i% (int)(pow(2,j)) == 0 ) {
                    s[undef_vars[j]] = 0;
                } else {
                    s[undef_vars[j]] = 1;
                }
            }
            if(!is_in_h_certificates(s)) {
                std::cout << "state ";
                for(size_t i = 0; i < s.size(); ++i) {
                    std::cout << (int)s[i];
                }
                std::cout << " is not in h certificate" << std::endl;
                return false;
                return false;
            }
            count++;
        }

        if(Cudd_NextCube(cubegen, &cube, &value_type) == 0) {
            done = true;
        }
    }
    //TODO: only check inductivity for those h_certificates which were actually used
    //(and inform if some were not used)

    //see if all h_certificates are inductive
    for(size_t i = 0; i < h_certificates.size(); ++i) {
        std::stringstream tmp;
        tmp << "checking if h certificate " << i << " is inductive";
        print_info(tmp.str());
        if(!h_certificates[i]->is_inductive()) {
            std::cout << "h certificate not inductive!" << std::endl;
            return false;
        }
    }

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
