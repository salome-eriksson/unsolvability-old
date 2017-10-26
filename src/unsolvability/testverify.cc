#include <istream>
#include <fstream>

#include "global_funcs.h"
#include "knowledgebase.h"
#include "statementcheckerbdd.h"
#include "statementcheckerhorn.h"
#include "stateset.h"
#include "task.h"
#include "cuddObj.hh"
#include "dddmp.h"

void createTaskFile() {
    //create task file
    std::cout << "creating task file" << std::endl;
    std::ofstream file; // out file stream
    file.open("task_testStatementChecker.txt");
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

}

void createKnowledgeBaseFile() {
    std::cout << "creating knowledge base file" << std::endl;
    std::ofstream file; // out file stream
    file.open("sets_testStatementChecker.txt");
    file << "stateset" << std::endl;
    file << "1 1 1 1 0" << std::endl;
    file << "1 1 0 1 0" << std::endl;
    file << "1 1 1 0 0" << std::endl;
    file << "1 1 0 0 0" << std::endl;
    file << "set end" << std::endl;
    file.close();
}

void createBDDS() {
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
    std::string filename = "bdds_testStatementChecker.txt";
    DdNode** bdd_arr = new DdNode*[bdds.size()];
    for(int i = 0; i < bdds.size(); ++i) {
        bdd_arr[i] = bdds[i].getNode();
    }
    FILE *fp;
    fp = fopen(filename.c_str(), "a");
    Dddmp_cuddBddArrayStore(manager.getManager(), NULL, bdds.size(), &bdd_arr[0], NULL,
                            NULL, NULL, DDDMP_MODE_TEXT, DDDMP_VARIDS, NULL, fp);
    fclose(fp);
}

void createInfoFileBDD() {
    std::cout << "creating info file" << std::endl;
    std::ofstream file; // out file stream
    file.open("info_testStatementCheckerBDD.txt");
    file << "0 1 2 3 4" << std::endl;
    file << "S1;S2;S3;S4;S5;S6;S7" << std::endl;
    file << "bdds_testStatementChecker.txt" << std::endl;
    file << "composite formulas begin" << std::endl;
    file << "S7 S5 not ^" << std::endl;
    file << "composite formulas end" << std::endl;
    file << "stmt_testStatementCheckerBDD.txt" << std::endl;
    file << "Statements:BDD end" << std::endl;
    file.close();
}

void createStatementFileBDD() {
    std::cout << "creating statement file BDD" << std::endl;
    std::ofstream file; // out file stream
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
    file.close();
}

void assertKBBDD(KnowledgeBase &kb) {
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

void createHornFormulas() {
    std::cout << "creating horn formulas" << std::endl;
    std::ofstream file; // out file stream
    file.open("horn_testStatementChecker.txt");
    file << "S1:0 1,-1|0 3,-1|1 2,-1|2 3,-1|1 3,-1|3,-1|"<< std::endl;
    file << "S2:,3|" << std::endl;
    file << "S2':3,-1|" << std::endl;
    file << "S3:0 1,-1|0 2,-1|0 4,-1|1 2,-1|2,-1|2 4,-1|" << std::endl;
    file << "S4:,2|" << std::endl;
    file << "S4':2,-1|" << std::endl;
    file << "S5:,2|,3|" << std::endl;
    file << "S5':2 3,-1|" << std::endl;
    file << "S6:0,-1|,2|" << std::endl;
    file << "S6':2,0|" << std::endl;
    file << "S7:,0|,1|4,-1|" << std::endl;
    file << "S8:0,-1|2,-1|2,-1" << std::endl;
    file.close();
}

void createInfoFileHorn() {
    std::cout << "creating info file" << std::endl;
    std::ofstream file; // out file stream
    file.open("info_testStatementCheckerHorn.txt");
    file << "horn_testStatementChecker.txt" << std::endl;
    file << "composite formulas begin" << std::endl;
    file << "S7 S5' ^" << std::endl;
    file << "composite formulas end" << std::endl;
    file << "stmt_testStatementCheckerHorn.txt" << std::endl;
    file << "Statements:Horn end" << std::endl;
    file.close();
}

void createStatementFileHorn() {
    std::cout << "creating statement file Horn" << std::endl;
    std::ofstream file; // out file stream
    file.open("stmt_testStatementCheckerHorn.txt");
    //until init:S7 this should be correct
    file << "sub:S5;S2" << std::endl;
    file << "sub:S6;S4" << std::endl;
    file << "exsub:S7 S5' ^;stateset" << std::endl;
    file << "exsub:empty;stateset" << std::endl;
    file << "prog:S1;S2' not" << std::endl;
    file << "reg:S3;S4' not" << std::endl;
    file << "in:0 1 0 0 1;S1" << std::endl;
    file << "init:S7" << std::endl;
    file << "prog:S8;true not" << std::endl;
    //from here on the statements should be false
    file << "sub:S1;S7" << std::endl;
    file << "exsub:S1;stateset" << std::endl;
    file << "prog:S1;S5' not" << std::endl;
    file << "reg:S3;S6' not" << std::endl;
    file << "in:1 0 0 1 0;S5" << std::endl;
    file << "init:S1" << std::endl;
    file.close();
}

void assertKBHorn(KnowledgeBase &kb) {
    assert(kb.is_subset("S5","S2"));
    assert(kb.is_subset("S6","S4"));
    assert(kb.is_subset("S7 S5' ^","stateset"));
    assert(kb.is_subset("empty","stateset"));
    assert(kb.is_progression("S1","S2' not"));
    assert(kb.is_regression("S3","S4' not"));
    Cube cube ={0,1,0,0,1};
    assert(kb.is_contained_in(cube,"S1"));
    assert(kb.contains_initial("S7"));
    assert(!kb.is_subset("S1","S7"));
    assert(!kb.is_subset("S1","stateset"));
    assert(!kb.is_progression("S1","S5' not"));
    assert(!kb.is_regression("S3","S6' not"));
    cube = {1,0,0,1,0};
    assert(!kb.is_contained_in(cube,"S5"));
    assert(!kb.contains_initial("S1"));
}

void testStatementChecker(std::string certtype) {
    createTaskFile();
    createKnowledgeBaseFile();
    std::ifstream in;
    if(certtype.compare("BDD")) {
        createBDDS();
        createInfoFileBDD();
        createStatementFileBDD();
        in.open("info_testStatementCheckerBDD.txt");
    } else if(certtype.compare("HORN")) {
        createHornFormulas();
        createInfoFileHorn();
        createStatementFileHorn();
        in.open("info_testStatementCheckerHorn.txt");
    }

    if(!in.is_open()) {
        exit_with(ExitCode::NO_CERTIFICATE_FILE);
    }

    std::cout << "creating Task Object" << std::endl;
    Task task("task_testStatementChecker.txt");
    std::cout << "creating KnowledgeBase Object" << std::endl;
    KnowledgeBase kb(&task,"sets_testStatementChecker.txt");
    StatementChecker *stmtchecker;
    if(certtype.compare("BDD")) {
        std::cout << "creating StatementCheckerBDD Object" << std::endl;
        stmtchecker = new StatementCheckerBDD(&kb,&task,in);
    } else if(certtype.compare("HORN")) {
        std::cout << "creating StatementCheckerHorn Object" << std::endl;
        stmtchecker = new StatementCheckerHorn(&kb,&task,in);
    }
    std::cout << "checking statements" << std::endl;
    stmtchecker->check_statements_and_insert_into_kb();

    if(certtype.compare("BDD")) {
        assertKBBDD(kb);
    } else if(certtype.compare("HORN")) {
        assertKBHorn(kb);
    }
}



int main(int , char** ) {
    testStatementChecker("BDD");
    testStatementChecker("HORN");
}
