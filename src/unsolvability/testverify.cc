#include <istream>
#include <fstream>

#include "global_funcs.h"
#include "knowledgebase.h"
#include "statementcheckerbdd.h"
#include "stateset.h"
#include "task.h"
#include "cuddObj.hh"
#include "dddmp.h"

void testStatementCheckerBDD() {

    //create BDDs and dump to file
    std::cout << "creating bdds" << std::endl;
    Cudd manager = Cudd(5,0);
    BDD a = manager.bddVar(0);
    BDD b = manager.bddVar(1);
    BDD c = manager.bddVar(2);
    BDD d = manager.bddVar(3);
    BDD e = manager.bddVar(4);
    std::vector<BDD> bdds;
    //S1 = (¬a ^ ¬c ^ ¬d) v (¬b ^ ¬d)
    //S2 = d
    //S3 = (¬a ^ ¬c) v (¬b ^ ¬c ^ ¬e)
    //S4 = c
    //S5 = (d ^ c)
    //S6 = (¬a ^ c)
    //S7 = (a ^ b ^ ¬e)
    bdds.push_back((!a * !c * !d) + (!b * !d));
    bdds.push_back(d);
    bdds.push_back((!a * !c) + (!b * !c * !e));
    bdds.push_back(c);
    bdds.push_back(d * c);
    bdds.push_back(!a * c);
    bdds.push_back(a * b * !e);

    //dump bdds in file
    std::cout << "dumping bdds" << std::endl;
    std::string filename = "bdds_testStatementCheckerBDD.txt";
    DdNode** bdd_arr = new DdNode*[bdds.size()];
    for(int i = 0; i < bdds.size(); ++i) {
        bdd_arr[i] = bdds[i].getNode();
    }
    FILE *fp;
    fp = fopen(filename.c_str(), "a");
    Dddmp_cuddBddArrayStore(manager.getManager(), NULL, bdds.size(), &bdd_arr[0], NULL,
                            NULL, NULL, DDDMP_MODE_TEXT, DDDMP_VARIDS, NULL, fp);
    fclose(fp);

    //create task file
    std::cout << "creating task file" << std::endl;
    std::ofstream file; // out file stream
    file.open("task_testStatementCheckerBDD.txt");
    file << "begin_atoms: 5" << std::endl;
    file << "a" << std::endl;
    file << "b" << std::endl;
    file << "c" << std::endl;
    file << "d" << std::endl;
    file << "e" << std::endl;
    file << "end_atoms" << std::endl;
    file << "begin_init" << std::endl;
    file << "0" << std::endl;
    file << "1" << std::endl;
    file << "end_init" << std::endl;
    file << "begin_goal" << std::endl;
    file << "1" << std::endl;
    file << "3" << std::endl;
    file << "4" << std::endl;
    file << "end_goal" << std::endl;
    file << "begin_actions:6" << std::endl;
    file << "begin_action" << std::endl;
    file << "a1" << std::endl;
    file << "cost: 1" << std::endl;
    file << "PRE:0" << std::endl;
    file << "PRE:1" << std::endl;
    file << "ADD:2" << std::endl;
    file << "ADD:3" << std::endl;
    file << "DEL:0" << std::endl;
    file << "end_action" << std::endl;
    file << "begin_action" << std::endl;
    file << "a2" << std::endl;
    file << "cost: 1" << std::endl;
    file << "PRE:1" << std::endl;
    file << "PRE:3" << std::endl;
    file << "ADD:0" << std::endl;
    file << "DEL:3" << std::endl;
    file << "end_action" << std::endl;
    file << "begin_action" << std::endl;
    file << "a3" << std::endl;
    file << "cost: 1" << std::endl;
    file << "PRE:2" << std::endl;
    file << "ADD:3" << std::endl;
    file << "DEL:2" << std::endl;
    file << "end_action" << std::endl;
    file << "begin_action" << std::endl;
    file << "a4" << std::endl;
    file << "cost: 1" << std::endl;
    file << "PRE:3" << std::endl;
    file << "PRE:4" << std::endl;
    file << "ADD:1" << std::endl;
    file << "DEL:4" << std::endl;
    file << "end_action" << std::endl;
    file << "begin_action" << std::endl;
    file << "a5" << std::endl;
    file << "cost: 1" << std::endl;
    file << "PRE:0" << std::endl;
    file << "PRE:2" << std::endl;
    file << "ADD:4" << std::endl;
    file << "end_action" << std::endl;
    file << "begin_action" << std::endl;
    file << "a6" << std::endl;
    file << "cost: 1" << std::endl;
    file << "PRE:1" << std::endl;
    file << "ADD:4" << std::endl;
    file << "DEL:1" << std::endl;
    file << "end_action" << std::endl;
    file << "end_actions" << std::endl;
    file.close();

    //create info file
    std::cout << "creating info file" << std::endl;
    file.open("info_testStatementCheckerBDD.txt");
    file << "0 1 2 3 4" << std::endl;
    file << "S1;S2;S3;S4;S5;S6;S7" << std::endl;
    file << "bdds_testStatementCheckerBDD.txt" << std::endl;
    file << "composite formulas begin" << std::endl;
    file << "S7 S5 not ^" << std::endl;
    file << "composite formulas end" << std::endl;
    file << "stmt_testStatementCheckerBDD.txt" << std::endl;
    file.close();

    //create statement file
    std::cout << "creating statement file" << std::endl;
    file.open("stmt_testStatementCheckerBDD.txt");
    //until init:S7 this should be correct
    file << "sub:S5;S2" << std::endl;
    file << "sub:S6;S4" << std::endl;
    file << "exsub:S7 S5 not ^;stateset" << std::endl;
    file << "exsub:empty;stateset" << std::endl;
    file << "prog:S1;S2" << std::endl;
    file << "reg:S3;S4" << std::endl;
    file << "in:0 1 0 0 1;S1" << std::endl;
    file << "init:S7" << std::endl;
    //from here on the statements should be false
    file << "sub:S1;S7" << std::endl;
    file << "exsub:S1;stateset" << std::endl;
    file << "prog:S1;S5" << std::endl;
    file << "reg:S3;S6" << std::endl;
    file << "in:1 0 0 1 0;S5" << std::endl;
    file << "init:S1" << std::endl;
    file << "statements end" << std::endl;
    file << "Statements:BDD end" << std::endl;
    file.close();

    //create knowledgebase file
    std::cout << "creating knowledge base file" << std::endl;
    file.open("sets_testStatementCheckerBDD.txt");
    file << "stateset" << std::endl;
    file << "1 1 1 1 0" << std::endl;
    file << "1 1 0 1 0" << std::endl;
    file << "1 1 1 0 0" << std::endl;
    file << "1 1 0 0 0" << std::endl;
    file << "set end" << std::endl;
    file.close();

    std::ifstream in;
    in.open("info_testStatementCheckerBDD.txt");
    if(!in.is_open()) {
        exit_with(ExitCode::NO_CERTIFICATE_FILE);
    }

    std::cout << "creating Task Object" << std::endl;
    Task task("task_testStatementCheckerBDD.txt");
    std::cout << "creating KnowledgeBase Object" << std::endl;
    KnowledgeBase kb(&task,"sets_testStatementCheckerBDD.txt");
    std::cout << "creating StatementCheckerBDD Object" << std::endl;
    StatementCheckerBDD stmtcheckerbdd(&kb,&task,in);
    std::cout << "checking statements" << std::endl;
    stmtcheckerbdd.check_statements();

    assert(kb.is_subset("S5","S2"));
    assert(kb.is_subset("S6","S4"));
    assert(kb.is_subset("S7 S5 not ^","stateset"));
    assert(kb.is_subset("empty","stateset"));
    assert(kb.is_progression("S1","S2"));
    assert(kb.is_regression("S3","S4"));
    Cube cube ={0,1,0,0,1};
    assert(kb.is_contained_in(cube,"S1"));
    assert(kb.contains_initial("S7"));
    assert(!kb.is_subset("S1","S7"));
    assert(!kb.is_subset("S1","stateset"));
    assert(!kb.is_progression("S1","S5"));
    assert(!kb.is_regression("S3","S6"));
    cube = {1,0,0,1,0};
    assert(!kb.is_contained_in(cube,"S5"));
    assert(!kb.contains_initial("S1"));
}



int main(int , char** ) {
    testStatementCheckerBDD();
}
