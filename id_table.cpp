/*
 * idtable.cpp
 *
 *  Created on: 12/1/2023
 *      Author: McHale Trotter
 */



#include <iostream>
#include <string>

#include "token.h"
#include "error_handler.h"
#include "id_table.h"
#include "id_table_entry.h"
#include "lille_kind.h"
#include "lille_type.h"


using namespace std;

//Constructor initializes the id_table with default values
id_table::id_table(error_handler* err)
{
   error = err;
   debug_mode = false;
   scope_level = 0;

   for(int i = 0; i < max_depth; i++)
   {
      sym_table[i] = new node;
      sym_table[i]->idt = NULL;
      sym_table[i]->left = NULL;
      sym_table[i]->right = NULL;
   }
}

//Increment the scope level when entering a new scope
void id_table::enter_scope()
{
   scope_level++;
   //sym_table[scope_level] = nullptr;
}

//Decrement the scope level when exiting a scope
void id_table::exit_scope()
{
   scope_level--;
}

//Get the current scope level
int id_table::scope()
{
   return scope_level;
}

//Look up an identifier in the symbol table based on its name
id_table_entry* id_table::lookup(string s)
{
   int scope = scope_level;
   node* ptr = sym_table[scope];
   bool found = false;

   while(scope >= 0)
   {
      if(ptr == NULL or ptr->idt == NULL)
      {
         if(scope > 0)
            ptr = sym_table[--scope];
         else
            scope--;
      }
      else if(s < ptr->idt->name())
      {
         ptr = ptr->left;
      }
      else if(s > ptr->idt->name())
      {
         ptr = ptr->right;
      }
      else if(s == ptr->idt->name())
      {
         if(debug_mode == true)
            cout << "ENTRY FOUND: found entry " << s << " of type " << ptr->idt->tipe().to_string() << endl;
            return ptr->idt;
      }
   }
   if(debug_mode == true)
      cout << "DID NOT FIND: Didn't find entry " << s << endl;
   return NULL;
}

//Look up an identifier in the symbol table based on a token
id_table_entry* id_table::lookup(token* tok)
{
   int scope = scope_level;
   node* ptr = sym_table[scope];
   bool found = false;

   while(scope >= 0)
   {
      if(ptr == NULL or ptr->idt == NULL)
      {
         if(scope > 0)
            ptr = sym_table[--scope];
         else
            scope--;
      }
      else if(tok < ptr->idt->token_value())
      {
         ptr = ptr->left;
      }
      else if(tok > ptr->idt->token_value())
      {
         ptr = ptr->right;
      }
      else if(tok == ptr->idt->token_value())
      {
         if(debug_mode == true)
            cout << "ENTRY FOUND: found entry " << tok->to_string() << " of type " << endl;
            return ptr->idt;
      }
   }
   if(debug_mode == true)
      cout << "DID NOT FIND: Didn't find entry " << tok->to_string() << endl;
   return NULL;
}

//Enable or disable tracing of all entries for debugging purposes
void id_table::trace_all(bool b)
{
   debug_mode = b;
}

//Check if tracing of all entries is enabled
bool id_table::trace_all()
{
   return debug_mode;
}



//Add an entry to the symbol table
void id_table::add_table_entry(id_table_entry* id)
{

	node* entry = new node;
	entry->idt = id;
	entry->right = NULL;
	entry->left = NULL;

	node* node1 = sym_table[scope()], * node2 = NULL;

	bool found = false;
	while(node1->idt != NULL)
	{
		node2 = node1;
		if(id->name() < node1->idt->name())
		{
			node1 = node1->left;
		}
		else
		{
			node1 = node1->right;
		}
		if(node1 == NULL)
			break;
	}

	if(node2 == NULL)  
		sym_table[scope()] = entry;
	else if (id->name() < node2->idt->name()) 
        node2->left = entry;
	else
		node2->right = entry;

	if(debug_mode)
		cout << "ADDED ENTRY: Created Entry " << id->name() << " in Scope " << scope() << endl;

}

//Enter a new identifier into the symbol table with specified attributes
id_table_entry* id_table::enter_id(token* id, lille_type typ, lille_kind kind, int level, int offset, lille_type return_tipe)
{
   id_table_entry* id_entry = new id_table_entry(id, typ, kind, level, offset, return_tipe);
   add_table_entry(id_entry/*, sym_table[scope_level]*/);
   return id_entry;
}

//Dump the entire symbol table for debugging purposes
void id_table::dump_id_table(bool dump_all)
{
   	node* ptr;
	if (!dump_all)
	{
		if(debug_mode)
		{
			cout << "Dump of idtable for current scope only." << endl;
			cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
		}

		ptr = sym_table[scope()];
		exit_scope();
		delete ptr;
		ptr = NULL;
	}
	else
	{
		if(debug_mode)
		{
			cout << "Dump of the entire symbol table." << endl;
			cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
		}
		
        	while(scope() > 0)
        	{
			ptr = sym_table[scope()];
			exit_scope();
			delete ptr;
			ptr = NULL;
		}
	}
}

//Recursive function to search for an identifier in the binary search tree
id_table::node* id_table::search_tree(string s, node* p)
{
   if (!p)
   {
      return nullptr;
   }

   if (s == p->idt->name())
   {
      return p;
   }

   if (s < p->idt->name())
   {
      return search_tree(s, p->right);
   }

   return search_tree(s, p->right);
}

void id_table::add_table_entry(id_table_entry* it, id_table::node* ptr)
{
   bool inserted = false;
   if(ptr != NULL)
   {
      string tab_name = it->token_value()->get_identifier_value();
      string temp;
      while(!inserted)
      {
         temp = ptr->idt->token_value()->get_identifier_value();
         if(tab_name == temp)
         {
            error->flag(it->token_value(), 82); //identifier declared multiple times
            break;
         }
         else if (tab_name < temp)
         {
            //insert left
            if (ptr->left == NULL)
            {
               node* n = new node;
               n->left = NULL;
               n->right = NULL;
               n->idt = it;
               ptr->left = n;
               inserted = true;
            }
            else
               ptr = ptr->left;
         }
         else
         {
            //insert right
            if (ptr->right == NULL)
            {
               node* n = new node;
               n->left = NULL;
               n->right = NULL;
               n->idt = it;
               ptr->right = n;
               inserted = true;
            }
            else
               ptr = ptr->right;
         }
      }
   }
}




//Recursive function to dump the binary search tree in-order
void id_table::dump_tree(node* ptr)
{
   if (ptr)
   {
      dump_tree(ptr->left);
      cout << ptr->idt->to_string() << endl;
      dump_tree(ptr->right);
   }
}

//Recursive function to dump additional information about identifiers in the binary search tree
void id_table::dump_all_ident(node* ptr)
{
   if(ptr)
   {
      dump_all_ident(ptr->left);
      cout << "Identifier: " << ptr->idt->name() << "   Type: " << ptr->idt->tipe().to_string() << endl;
      dump_all_ident(ptr->right);
   }

}



























