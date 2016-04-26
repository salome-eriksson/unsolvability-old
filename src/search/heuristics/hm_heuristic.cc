#include "hm_heuristic.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"

#include <cassert>
#include <limits>
#include <set>

using namespace std;


namespace HMHeuristic {
HMHeuristic::HMHeuristic(const Options &opts)
    : Heuristic(opts),
      m(opts.get<int>("m")),
      has_cond_effects(has_conditional_effects(task_proxy)) {
}


HMHeuristic::~HMHeuristic() {
}


bool HMHeuristic::dead_ends_are_reliable() const {
    return !has_axioms(task_proxy) && !has_cond_effects;
}


void HMHeuristic::initialize() {
    cout << "Using h^" << m << "." << endl;
    cout << "The implementation of the h^m heuristic is preliminary." << endl
         << "It is SLOOOOOOOOOOOW." << endl
         << "Please do not use this for comparison!" << endl;
    generate_all_tuples();

    cudd_manager = new CuddManager();
    int count = 0;
    neg_none.resize(task_proxy.get_variables().size(), -1);
    for(VariableProxy vars: task_proxy.get_variables()) {
        std::string name = vars.get_fact(vars.get_domain_size()-1).get_name();
        if(name.substr(0,7).compare("Negated") == 0 || name.substr(0,5).compare("<none") == 0) {
            neg_none[count] = vars.get_domain_size()-1;
        }
        count++;
    }
}


int HMHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    if (is_goal_state(task_proxy, state)) {
        return 0;
    } else {
        Tuple s_tup;
        state_to_tuple(state, s_tup);

        init_hm_table(s_tup);
        update_hm_table();
        //dump_table();

        Tuple goals;
        for (FactProxy goal : task_proxy.get_goals()) {
            goals.emplace_back(goal.get_variable().get_id(), goal.get_value());
        }
        int h = eval(goals);

        if (h == numeric_limits<int>::max())
            return DEAD_END;
        return h;
    }
}


void HMHeuristic::init_hm_table(const Tuple &t) {
    for (auto &hm_ent : hm_table) {
        const Tuple &tuple = hm_ent.first;
        int h_val = check_tuple_in_tuple(tuple, t);
        hm_table[tuple] = h_val;
    }
}


void HMHeuristic::update_hm_table() {
    int round = 0;
    do {
        ++round;
        was_updated = false;

        for (OperatorProxy op : task_proxy.get_operators()) {
            Tuple pre;
            get_operator_pre(op, pre);

            int c1 = eval(pre);
            if (c1 != numeric_limits<int>::max()) {
                Tuple eff;
                get_operator_eff(op, eff);
                vector<Tuple> partial_effs;
                generate_all_partial_tuples(eff, partial_effs);
                for (Tuple &partial_eff : partial_effs) {
                    update_hm_entry(partial_eff, c1 + op.get_cost());

                    int eff_size = partial_eff.size();
                    if (eff_size < m) {
                        extend_tuple(partial_eff, op);
                    }
                }
            }
        }
    } while (was_updated);
}


void HMHeuristic::extend_tuple(const Tuple &t, const OperatorProxy &op) {
    for (auto &hm_ent : hm_table) {
        const Tuple &tuple = hm_ent.first;
        bool contradict = false;
        for (auto &fact : tuple) {
            if (contradict_effect_of(op, fact.first, fact.second)) {
                contradict = true;
                break;
            }
        }
        if (!contradict && (tuple.size() > t.size()) && (check_tuple_in_tuple(t, tuple) == 0)) {
            Tuple pre;
            get_operator_pre(op, pre);

            Tuple others;
            for (auto &fact : tuple) {
                if (find(t.begin(), t.end(), fact) == t.end()) {
                    others.push_back(fact);
                    if (find(pre.begin(), pre.end(), fact) == pre.end()) {
                        pre.push_back(fact);
                    }
                }
            }

            sort(pre.begin(), pre.end());


            set<int> vars;
            bool is_valid = true;
            for (auto &fact : pre) {
                if (vars.count(fact.first) != 0) {
                    is_valid = false;
                    break;
                }
                vars.insert(fact.first);
            }

            if (is_valid) {
                int c2 = eval(pre);
                if (c2 != numeric_limits<int>::max()) {
                    update_hm_entry(tuple, c2 + op.get_cost());
                }
            }
        }
    }
}


int HMHeuristic::eval(const Tuple &t) const {
    vector<Tuple> partial;
    generate_all_partial_tuples(t, partial);
    int max = 0;
    for (Tuple &tuple : partial) {
        assert(hm_table.count(tuple) == 1);

        int h = hm_table.at(tuple);
        if (h > max) {
            max = h;
        }
    }
    return max;
}


int HMHeuristic::update_hm_entry(const Tuple &t, int val) {
    assert(hm_table.count(t) == 1);
    if (hm_table[t] > val) {
        hm_table[t] = val;
        was_updated = true;
    }
    return val;
}


int HMHeuristic::check_tuple_in_tuple(
    const Tuple &tup, const Tuple &big_tuple) const {
    for (auto &fact0 : tup) {
        bool found = false;
        for (auto &fact1 : big_tuple) {
            if (fact0 == fact1) {
                found = true;
                break;
            }
        }
        if (!found) {
            return numeric_limits<int>::max();
        }
    }
    return 0;
}


void HMHeuristic::state_to_tuple(const State &state, Tuple &t) const {
    for (FactProxy fact : state) {
        t.emplace_back(fact.get_variable().get_id(), fact.get_value());
    }
}


void HMHeuristic::get_operator_pre(const OperatorProxy &op, Tuple &t) const {
    for (FactProxy pre : op.get_preconditions()) {
        t.emplace_back(pre.get_variable().get_id(), pre.get_value());
    }
    sort(t.begin(), t.end());
}


void HMHeuristic::get_operator_eff(const OperatorProxy &op, Tuple &t) const {
    for (EffectProxy eff : op.get_effects()) {
        FactProxy fact = eff.get_fact();
        t.emplace_back(fact.get_variable().get_id(), fact.get_value());
    }
    sort(t.begin(), t.end());
}


bool HMHeuristic::contradict_effect_of(
    const OperatorProxy &op, int var, int val) const {
    for (EffectProxy eff : op.get_effects()) {
        FactProxy fact = eff.get_fact();
        if (fact.get_variable().get_id() == var && fact.get_value() != val) {
            return true;
        }
    }
    return false;
}


void HMHeuristic::generate_all_tuples() {
    Tuple t;
    generate_all_tuples_aux(0, m, t);
}


void HMHeuristic::generate_all_tuples_aux(int var, int sz, const Tuple &base) {
    int num_variables = task_proxy.get_variables().size();
    for (int i = var; i < num_variables; ++i) {
        int domain_size = task_proxy.get_variables()[i].get_domain_size();
        for (int j = 0; j < domain_size; ++j) {
            Tuple tup(base);
            tup.emplace_back(i, j);
            hm_table[tup] = 0;
            if (sz > 1) {
                generate_all_tuples_aux(i + 1, sz - 1, tup);
            }
        }
    }
}


void HMHeuristic::generate_all_partial_tuples(
    const Tuple &base_tuple, vector<Tuple> &res) const {
    Tuple t;
    generate_all_partial_tuples_aux(base_tuple, t, 0, m, res);
}


void HMHeuristic::generate_all_partial_tuples_aux(
    const Tuple &base_tuple, const Tuple &t, int index, int sz, vector<Tuple> &res) const {
    if (sz == 1) {
        for (size_t i = index; i < base_tuple.size(); ++i) {
            Tuple tup(t);
            tup.push_back(base_tuple[i]);
            res.push_back(tup);
        }
    } else {
        for (size_t i = index; i < base_tuple.size(); ++i) {
            Tuple tup(t);
            tup.push_back(base_tuple[i]);
            res.push_back(tup);
            generate_all_partial_tuples_aux(base_tuple, tup, i + 1, sz - 1, res);
        }
    }
}


void HMHeuristic::dump_table() const {
    for (auto &hm_ent : hm_table) {
        cout << "h[";
        dump_tuple(hm_ent.first);
        cout << "] = " << hm_ent.second << endl;
    }
}


void HMHeuristic::dump_tuple(const Tuple &tup) const {
    cout << tup[0].first << "=" << tup[0].second;
    for (size_t i = 1; i < tup.size(); ++i)
        cout << "," << tup[i].first << "=" << tup[i].second;
}

void HMHeuristic::build_mutex_bdds() {
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        int amount_vals = g_variable_domain[i];
        // if the last value of the variable is not a bdd_variable, it is
        // either "none of those" or "NegatedAtom" --> ignore that value
        if(!cudd_manager->isVariable(i, g_variable_domain[i]-1)) {
            amount_vals-=1;
        }
        for(int j = 0; j < amount_vals; ++j) {
            CuddBDD bdd_j(cudd_manager, false);
            bdd_j.lor_bddvar(i,j,true);
            for(int k = j+1; k < amount_vals; ++k) {
                CuddBDD* res = new CuddBDD(bdd_j);
                res->lor_bddvar(i, k, true);
                mutex_bdds.push_back(res);
            }
        }
    }
}

void HMHeuristic::build_unsolvability_certificate(const GlobalState &s) {
    if(mutex_bdds.empty()) {
        build_mutex_bdds();
    }
    //see if the state is covered by an already existing certificate
    CuddBDD statebdd(cudd_manager, s);
    for(auto vec : certificates) {
        assert(vec.size()>0);
        bool covered = true;
        for(CuddBDD* bdd : vec) {
            if(!statebdd.isSubsetOf(*bdd)) {
                covered = false;
                break;
            }
        }
        if(covered) {
            return;
        }
    } // end looping over certificates

    std::vector<CuddBDD*> new_cert(mutex_bdds);

    //empty vector (needed for reference)
    std::vector<std::pair<int,int>> emptyvec;

    for(auto e : hm_table) {
        if(e.second < numeric_limits<int>::max()) {
            continue;
        }

        // check if this tuple is already subsumed by another tuple with infinite heuristic value
        // TODO: const reference would be better but general_all_partial_tuples()   does not take const references
        Tuple tuple(e.first);
        vector<Tuple> partial_tuples;
        generate_all_partial_tuples(tuple, partial_tuples);
        bool subsumed = false;
        for(Tuple t : partial_tuples) {
            if(t == tuple) {
                continue;
            }
            if(hm_table[t] == numeric_limits<int>::max()) {
                subsumed = true;
                break;
            }
        }
        if(subsumed) {
            continue;
        }
        // for some reason, hm also combines tuples where one or several variables are
        // of type "NegatedAtom" or "<none of those">
        // we simply ignore those tuples
        // TODO: ask Malte if this is correct behaviour
        if(contains_negated_or_none_of_those(tuple)) {
            continue;
        }

        // build bdd for negated tuple
        CuddBDD* bdd = new CuddBDD(cudd_manager, tuple, emptyvec);
        bdd->negate();
        new_cert.push_back(bdd);
    }

    certificates.push_back(new_cert);
}

int HMHeuristic::get_number_of_unsolvability_certificates() {
    return certificates.size();
}

void HMHeuristic::write_subcertificates(ofstream &cert_file) {
    if(certificates.empty()) {
        return;
    }
    for(size_t i = 0; i < certificates.size(); ++i) {
        std::string name = "cert_hm"+std::to_string(i);
        std::string txtname = name+".txt";
        std::vector<std::string> names(certificates[i].size(), "");
        for(size_t j = 0; j < names.size(); ++j) {
            names[j] = name+"_"+std::to_string(j);
        }
        cudd_manager->dumpBDDs(certificates[i], names, txtname);
        cert_file << "strong_conjunctive_certificate\n";
        cert_file << "File:" << txtname << "\n";
        cert_file << "begin_variables\n";
        cudd_manager->writeVarOrder(cert_file);
        cert_file << "end_variables\n";
        cert_file << "end_certificate\n";
    }
}

bool HMHeuristic::contains_negated_or_none_of_those(Tuple &tuple) {
    for(std::pair<int,int> t : tuple) {
        if(neg_none[t.first] == t.second) {
            return true;
        }
    }
    return false;
}


static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("h^m heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "ignored");
    parser.document_language_support("axioms", "ignored");
    parser.document_property("admissible",
                             "yes for tasks without conditional "
                             "effects or axioms");
    parser.document_property("consistent",
                             "yes for tasks without conditional "
                             "effects or axioms");

    parser.document_property("safe",
                             "yes for tasks without conditional "
                             "effects or axioms");
    parser.document_property("preferred operators", "no");

    parser.add_option<int>("m", "subset size", "2", Bounds("1", "infinity"));
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new HMHeuristic(opts);
}

static Plugin<Heuristic> _plugin("hm", _parse);
}
