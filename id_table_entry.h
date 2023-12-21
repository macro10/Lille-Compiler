#include <iostream>
#include <string>

#include "token.h"
#include "lille_type.h"
#include "lille_kind.h"
#include "id_table.h"

#ifndef ID_TABLE_ENTRY_H_
#define ID_TABLE_ENTRY_H_

using namespace std;

class id_table_entry {
private:
   token* id_entry;
   int lev_entry;
   int offset_entry;
   lille_kind kind_entry;
   bool trace_entry;
   lille_type typ_entry;
   int i_val_entry;
   float r_val_entry;
   string s_val_entry;
   bool b_val_entry;
   id_table_entry* p_list_entry;
   int n_par_entry;
   lille_type r_ty_entry;

public:
   id_table_entry();
   id_table_entry(token* id, lille_type typ = lille_type::type_unknown, lille_kind kind = lille_kind::unknown, int level = 0, int offset = 0, lille_type return_tipe = lille_type::type_unknown);

   void trace_obj(bool trac);
   bool trace();
   int offset();
   int level();
   lille_kind kind();
   lille_type tipe();
   token* token_value();
   string name();
   int integer_value();
   float real_value();
   string string_value();
   bool bool_value();
   lille_type return_tipe();

   void fix_const(int integer_value = 0, float real_value = 0, string string_value = "", bool bool_value = false);
   void fix_return_type(lille_type ret_ty);
   void add_param(id_table_entry* param_entry);
   id_table_entry* nth_parameter(int n);
   int number_of_params();
   string to_string();

};

#endif /* ID_TABLE_ENTRY_H_ */
