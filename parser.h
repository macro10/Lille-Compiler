/*
   parser.h
   Name: McHale Trotter
*/

#ifndef PARSER_H_
#define PARSER_H_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "symbol.h"
#include "token.h"
#include "error_handler.h"
#include "id_table.h"
#include "scanner.h"

using namespace std;

class parser {
private:
	bool debugging {false};

	token* current_token;
	error_handler* error;
	scanner* scan;
        id_table* id_tab;
	bool typeFlag{false};
	parser(); //default constructor for the parser.
        id_table_entry* curr_entry;
        id_table_entry* curr_func_proc;
        id_table_entry* curr_ident;

public:
	bool eof_flag;
	//boolean flag for if the parser reaches the eof marker.

	parser(scanner* s, error_handler* e, id_table* i);

        void program();
	void prog();
	void define_function(string name, lille_type x, lille_type y);
	void block();
	vector<token*> identList();
	void decl();
	lille_type type();
	void paramList();
	void param();
	void paramKind();

	void statementList();
	void statement();
	void simpleStatement();
	void compoundStatement();
	void ifStatement();
	void whileStatement();
	void forStatement();
	void loopStatement();
	void range();

	void expr();
	void simpleExpr();
	void relOp();
	void expr2();
	void term();

	void factor();
	void primary();

};

#endif
