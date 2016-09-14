#include "search_certificate.h"
#include "simple_certificate.h"
#include "strong_conjunctive_certificate.h"
#include <iomanip>

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
    std::cout << "Amount of subcertificates: " << subcertificates_amount << std::endl;
    h_certificates.resize(subcertificates_amount);
    for(int i = 0; i < subcertificates_amount; ++i) {
        std::getline(in, line);

        if(line.compare("simple_certificate") == 0) {
            h_certificates[i]= new SimpleCertificate(task, in);
        } else if(line.compare("strong_conjunctive_certificate") == 0) {
            h_certificates[i]= new StrongConjunctiveCertificate(task, in);
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
}

/* The certificate contains a state if it is either in bdd_exp or bdd_pr */
// TODO: technically the certificate also contains a state if it is contained in
// one of the h_certificates... but since we currently only need the function for
// the initial state, this is not implemented yet.
bool SearchCertificate::contains_state(const Cube &state) {
    BDD statebdd = build_bdd_from_cube(state);

    if(statebdd.Leq(bdd_exp)) {
        return true;
    } else if(statebdd.Leq(bdd_pr)) {
        return true;
    }
    return false;
}

/* We test simultaneously whether bdd_exp or any of the h_certificates contain
   the goal --> we also do one part of verifying whether the h_certificates are
   actual certificates */
bool SearchCertificate::contains_goal() {
    BDD goalbdd = build_bdd_from_cube(task->get_goal());
    goalbdd = goalbdd * bdd_exp;
    if(!goalbdd.IsZero()) {
        return true;
    }
    for(size_t i = 0; i < h_certificates.size(); ++i) {
        if(h_certificates[i]->contains_goal()) {
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

    double action_check = timer();

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

        // if succ is not a subset of union_primed, it contains successors which
        // are neither in bdd_exp nor bdd_pr --> not inductive
        if(!succ.Leq(union_primed)) {
            std::cout << "action " << i << " is not inductive" << std::endl;
            return false;
        }
    } // end loop over actions
    action_check = timer() - action_check;
    // info output
    print_info("Finished checking actions");


    // if there are no pruned states, bbd_exp is inductive already --> we are done
    if(bdd_pr.IsZero()) {
        print_info("bdd_pr is empty - no further checking needed");
        print_statistics(action_check, 0.0, 0.0);
        return true;
    }


    // SECOND PART: see if all states in bdd_pr are covered by h_certificates
    print_info("checking if pruned states are contained in h certificate");
    double pr_check = timer();
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
                dont_cares.push_back(bddvar_to_global_fact[2*i]);
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
                print_statistics(action_check, (timer()-pr_check), 0.0);
                return false;
            }
            count++;
        }

        if(Cudd_NextCube(cubegen, &cube, &value_type) == 0) {
            done = true;
        }
    }
    pr_check = timer() - pr_check;
    // info output
    print_info("finished checking states");
    std::cout << "Amount of states in bdd_pr: " << count << std::endl;
    // TODO: only check inductivity for those h_certificates which were actually used
    // (and inform if some were not used)

    double hcert_check = timer();
    // THIRD PART: see if all h_certificates are inductive
    /* remark: inductiveness is the only part not verified yet to show that the
       h_certificates are actual certificates; we checked already if all states
       in bdd_pr are included in some h_certificate (see SECOND PART) and that
       no certificate contains a goal state (see function contains_goal()) */
    for(size_t i = 0; i < h_certificates.size(); ++i) {
        // info output
        std::stringstream tmp;
        tmp << "checking if h certificate " << i << " is inductive";
        print_info(tmp.str());

        if(!h_certificates[i]->is_inductive()) {
            std::cout << "h certificate not inductive!" << std::endl;
            print_statistics(action_check, pr_check, (timer()-hcert_check));
            return false;
        }
    }
    hcert_check = timer() - hcert_check;
    print_statistics(action_check, pr_check, hcert_check);
    // all test passed, the certificate is valid
    return true;
}

bool SearchCertificate::is_in_h_certificates(Cube& s) {
    for(size_t i = 0; i < h_certificates.size(); ++i) {
        if(h_certificates[i]->contains_state(s)) {
            return true;
        }
    }
    return false;
}

void SearchCertificate::print_statistics(double action_check, double pr_check, double hcert_check) {
    std::cout << "Time for part 1: " << std::fixed << std::setprecision(2) << action_check << std::endl;
    std::cout << "Time for part 2: " << std::fixed << std::setprecision(2) << pr_check << std::endl;
    std::cout << "Time for part 3: " << std::fixed << std::setprecision(2) << hcert_check << std::endl;
}
