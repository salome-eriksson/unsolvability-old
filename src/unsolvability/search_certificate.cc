#include "search_certificate.h"
#include "simple_certificate.h"

#include "math.h"
#include <cassert>
#include <fstream>

SearchCertificate::SearchCertificate(Task *task, std::ifstream &in)
     : Certificate(task) {

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
    std::cout << "reading in variable order for search certificate" << std::endl;
    map_global_facts_to_bdd_var(in);
    std::vector<BDD> bddvec;
    std::cout << "parsing bdds in search certificate file" << std::endl;
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
    std::cout << "done building search certificate" << std::endl;
}

/* The certificate contains a state if it is either in the BDD representing
   the expanded states or in one of the h_certificates */

bool SearchCertificate::contains_state(const State &state) {
    BDD statebdd = manager.bddOne();
    for(size_t i = 0; i < state.size(); ++i) {
      if(state[i]) {
        statebdd = statebdd * manager.bddVar(fact_to_bddvar[i]);
      } else {
        statebdd = statebdd - manager.bddVar(fact_to_bddvar[i]);
      }
    }
    BDD result = bdd_exp * statebdd;
    if(!result.IsZero()) {
      return true;
    }
    for(size_t i = 0; i < h_certificates.size(); ++i) {
        if(h_certificates[i]->contains_state(state)) {
            return true;
        }
    }
    return false;
}


bool SearchCertificate::contains_goal() {
    BDD goalbdd = manager.bddOne();
    const std::vector<int> &goal = task->get_goal();
    for(size_t i = 0; i < goal.size(); ++i) {
      goalbdd = goalbdd * manager.bddVar(fact_to_bddvar[goal[i]]);
    }
    BDD result = bdd_exp * goalbdd;
    if (!result.IsZero()) {
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
    //first part: check if all successors are contained in bdd_exp \union bdd_pr

    //permutation for renaming the certificate to the primed variables
    int factamount = task->get_number_of_facts();
    int permutation[factamount*2];
    for(int i = 0 ; i < factamount; ++i) {
      permutation[2*i] = (2*i)+1;
      permutation[(2*i)+1] = 2*i;
    }
    BDD exp_primed = bdd_exp.Permute(permutation);
    BDD pr_primed = bdd_pr.Permute(permutation);

    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        const Action &a = task->get_action(i);
        BDD action_bdd = build_bdd_for_action(a);

        // succ represents pairs of states and successors with action a
        BDD succ = action_bdd * bdd_exp;
        // new_states contains pairs of states from above where
        // the successor is not in c (primed)
        BDD new_states = succ - exp_primed;
        BDD res = new_states - pr_primed;
        // res contains all pairs of states where the successor is in neither c nor bdd_pr
        // if it is not empty, then a successor from a state of c
        // is not covered anywhere and thus the certificate is not inductive
        if(!res.IsZero()) {
            return false;
        }
    } //end loop over actions


    //if there are no pruned states, bbd_exp is inductive already --> we are done
    if(bdd_pr.IsZero()) {
        return true;
    }


    //second part: see if all states in bdd_pr are covered by h_certificates
    //and all h_certificates are inductive

    //the bdd contains all facts from the task, plus their primed version
    int* cube;
    CUDD_VALUE_TYPE value_type;
    DdGen * cubegen = bdd_pr.FirstCube(&cube, &value_type);
    bool done = false;

    //loop over all states in bdd_pr and see if they are in any h_certificate
    while(!done) {
        State s = State(factamount, false);
        std::vector<int> undef_vars;
        //the 2*i is because we are only interested in unprimed (=even) variables
        //(the uneven variables are all primed)
        for(int i = 0; i < factamount; ++i) {
            if(cube[2*i] == 1) {
                assert(bdd_to_global_var_mapping[2*i] != -1);
                s[bdd_to_global_var_mapping[2*i]] = true;
            // if a variable is undefined, we need to try it with both true and false
            // in a first pass, set all undefined variables to false, then try out
            // all other combinations later
            } else if(cube[2*i] == 2) {
                undef_vars.push_back(bdd_to_global_var_mapping[2*i]);
            } //cube[2*i] = 0 -> false (which was predefined already)
        }

        if(!is_in_h_certificates(s)) {
            return false;
        }

        //also try out all combinations of true/false for the undefined variables
        //TODO: test this!
        for(int i = 1; i < pow(2,undef_vars.size()); ++i) {
            for(size_t j = 0; j < undef_vars.size(); ++j) {
                if(i% (int)(pow(2,j)) == 0 ) {
                    s[undef_vars[j]] = false;
                } else {
                    s[undef_vars[j]] = true;
                }
            }
            if(!is_in_h_certificates(s)) {
                return false;
            }
        }

        if(Cudd_NextCube(cubegen, &cube, &value_type) == 0) {
            done = true;
        }
    }
    //TODO: only check inductivity for those h_certificates which were actually used
    //(and inform if some were not used)

    //see if all h_certificates are inductive
    for(size_t i = 0; i < h_certificates.size(); ++i) {
        if(!h_certificates[i]->is_inductive()) {
            return false;
        }
    }

    return true;
}

bool SearchCertificate::is_in_h_certificates(State& s) {
    for(size_t i = 0; i < h_certificates.size(); ++i) {
        if(h_certificates[i]->contains_state(s)) {
            return true;
        }
    }
    return false;
}
