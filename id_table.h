/*
 * idtable.h
 *
 *  Created on: Jun 18, 2020
 *      Author: Michael Oudshoorn
 */

#ifndef ID_TABLE_H_
#define ID_TABLE_H_

#include <iostream>
#include <string>

#include "token.h"
#include "error_handler.h"
#include "id_table_entry.h"
#include "lille_type.h"
#include "lille_kind.h"


using namespace std;

class id_table {
private:
   error_handler* error;

   bool debug_mode;
   int scope_level;

   static const int max_depth = 1000; // maximum depth of nesting permitted in source code
   struct node {
   node* left;
   node* right;
   id_table_entry* idt;
   };

   node* sym_table[max_depth];
   node* search_tree(string s, node* p);
   void add_table_entry(id_table_entry* it, node* ptr);
   void dump_tree(node* ptr);

public:
   id_table(error_handler* err);
    
   void enter_scope();
   void exit_scope();
   int scope();

   id_table_entry* lookup(string s);
   id_table_entry* lookup(token* tok);
   void trace_all(bool b);
   bool trace_all();

   void add_table_entry(id_table_entry* it);


   id_table_entry* enter_id(token* id, lille_type typ = lille_type::type_unknown, lille_kind kind = lille_kind::unknown, int level = 0, int offset = 0, lille_type return_tipe = lille_type::type_unknown);

   void dump_id_table(bool dump_all = true);

   void dump_all_ident(node* ptr);




};

#endif /* ID_TABLE_H_ */
