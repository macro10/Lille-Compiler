#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <cctype>
#include <cmath>
#include <vector>
#include "symbol.h"
#include "scanner.h"
#include "error_handler.h"
#include "parser.h"
#include "token.h"
#include "lille_exception.h"
#include "id_table.h"
#include "id_table_entry.h"

// Constructor for the parser class, initializing member variables.
parser::parser(scanner* scan, error_handler* err, id_table* id_tab)
{
   this->scan = scan;
   this->error = error;
   this->id_tab = id_tab;

   curr_entry = NULL;
   curr_ident = NULL;
   curr_func_proc = NULL;

}

// Define a function in the symbol table with its name, return type, and argument type.
void parser::define_function(string name, lille_type x, lille_type y)
{
   token* fun, * arg;
   symbol* sym;
   id_table_entry* fun_id, * param_id;

   // Create a symbol and token for the function name.
   sym = new symbol(symbol::identifier);
   fun = new token(sym, 0, 0);
   fun->set_identifier_value(name);

   // Enter the function into the symbol table.
   fun_id = id_tab->enter_id(fun, lille_type::type_func, lille_kind::unknown, 0, 0, x);
   id_tab->add_table_entry(fun_id);

   // Create a token for the function argument.
   arg = new token(sym, 0, 0);
   arg->set_identifier_value("__" + name + "_arg__"); 

   // Enter the argument into the symbol table as a parameter of the function.
   param_id = new id_table_entry(arg, y, lille_kind::value_param, 0, 0, lille_type::type_unknown);
   fun_id->add_param(param_id);

}

// Program entry point in the parser.
void parser::prog()
{
   if(debugging)
      cout << "Parser: entering prog()" << endl;

   // Ensure the program starts with the "program" symbol
   scan->must_be(symbol::program_sym);

   // Create a token for the program name and enter it into the symbol table.
   symbol* sym = new symbol(symbol::program_sym);
   token* prog = new token(sym, 0, 0);
   prog->set_prog_value(scan->get_current_identifier_name());
   id_table_entry* prog_id = id_tab->enter_id(prog, lille_type::type_prog, lille_kind::unknown, id_tab->scope(), 0, lille_type::type_unknown);
   id_tab->add_table_entry(prog_id);
   curr_entry = prog_id;


   // Ensure the program name is an identifier.
   scan->must_be(symbol::identifier);

   // Define built-in functions.
   parser::define_function("INT2REAL", lille_type::type_real, lille_type::type_integer);
   parser::define_function("REAL2INT", lille_type::type_integer, lille_type::type_real);
   parser::define_function("INT2STRING", lille_type::type_string, lille_type::type_integer);
   parser::define_function("REAL2STRING", lille_type::type_string, lille_type::type_real);

   // Ensure the program declaration is followed by "is".
   scan->must_be(symbol::is_sym);

   // Process the block of the program.
   block();

   // Ensure the program ends with a semicolon and the end of the program symbol.
   scan->must_be(symbol::semicolon_sym);
   scan->must_be(symbol::end_of_program);

   if(debugging)
      cout << "Parser: exiting prog()" << endl;
}

// Process a block of code.
void parser::block()
{
   if(debugging)
      cout << "Parser: entering block()" << endl;

   // Process declarations until "begin" is encountered.
   while (scan->have(symbol::identifier) || scan->have(symbol::function_sym) || scan->have(symbol::procedure_sym))
   {
      decl();
   }

   // Ensure the block starts with "begin".
   scan->must_be(symbol::begin_sym);

   // Process statements.
   statementList();

   // Ensure the block ends with "end".
   scan->must_be(symbol::end_sym);

   // If an identifier follows "end", consume it.
   if (scan->have(symbol::identifier))
   {
      scan->must_be(symbol::identifier);
   }

   // Exit the current scope.
   id_tab->exit_scope();

   if(debugging)
      cout << "Parser: exiting block()" << endl;
}

// Process a list of identifiers
vector<token*> parser::identList()
{
   if(debugging)
      cout << "Parser: entering identList()" << endl;
 
   vector<token*> tokens;
   if(scan->have(symbol::identifier))
   {
      tokens.push_back(scan->this_token());
      scan->must_be(symbol::identifier);
   }

   // Process comma-separated identifiers.
   while(scan->have(symbol::comma_sym))
   {
      scan->must_be(symbol::comma_sym);
      tokens.push_back(scan->this_token());
      scan->must_be(symbol::identifier);
   }

   if(debugging)
      cout << "Parser: exiting identList()" << endl;

   return tokens;
}

// Process a declaration
void parser::decl()
{
   if(debugging)
      cout << "Parser: entering decl()" << endl;

   vector<token*> tokens;
   bool comma_found = false;
   lille_type ty;

   // Process a procedure declaration.
   if(scan->have(symbol::procedure_sym))
   {
      scan->must_be(symbol::procedure_sym);
      scan->must_be(symbol::identifier);
      
      // Process procedure parameters if present
      if(scan->have(symbol::left_paren_sym))
      {
         scan->must_be(symbol::left_paren_sym);
         id_tab->enter_scope();
         paramList();
         scan->must_be(symbol::right_paren_sym);
      }

      scan->must_be(symbol::is_sym);
      block();

   }
   // Process a function declaration
   else if(scan->have(symbol::function_sym))
   {
      scan->must_be(symbol::function_sym);
      scan->must_be(symbol::identifier);
      
      // Process function parameters
      if(scan->have(symbol::left_paren_sym))
      {
         scan->must_be(symbol::left_paren_sym);
         id_tab->enter_scope();
         paramList();
         scan->must_be(symbol::right_paren_sym);
      }

      scan->must_be(symbol::return_sym);
      type();
      scan->must_be(symbol::is_sym);
      block();

   }
   // Process variable or constant declaration.
   else if (scan->have(symbol::identifier))
   {
      bool const_decl = false;
      float r_const;
      int i_const;
      std::string s_const;
      bool b_const;
      id_table_entry* id_tab_ent;
      tokens = identList();
      
      scan->must_be(symbol::colon_sym);
      if(scan->have(symbol::constant_sym))
      {
         const_decl = true;
         scan->must_be(symbol::constant_sym);
      }
      ty = type();
      if(scan->have(symbol::becomes_sym))
      {
         scan->must_be(symbol::becomes_sym);
      
         if(scan->have(symbol::integer))
         {
            i_const = scan->this_token()->get_integer_value();
            if(!ty.is_type(lille_type::type_integer))
               error->flag(scan->this_token(), 111); //const expr doesn't match its type declaration
            scan->must_be(symbol::integer);
         }
         else if(scan->have(symbol::real_num))
         {
            r_const = scan->this_token()->get_real_value();
            if (!ty.is_type(lille_type::type_real))
               error->flag(scan->this_token(), 111); //const expr doesn't match its type declaration
            scan->must_be(symbol::real_num);
         }
         else if(scan->have(symbol::strng))
         {
            s_const = scan->this_token()->get_string_value();
            if (!ty.is_type(lille_type::type_string))
               error->flag(scan->this_token(), 111); //const expr doesn't match its type declaration
            scan->must_be(symbol::strng);
         }
         else if (scan->have(symbol::false_sym))
         {
            b_const = false;
            if (!ty.is_type(lille_type::type_boolean))
               error->flag(scan->this_token(), 111); //const expr doesn't match its type declaration
            scan->must_be(symbol::boolean_sym);
         }
         else if (scan->have(symbol::true_sym))
         {
            b_const = true;
            if (!ty.is_type(lille_type::type_boolean))
               error->flag(scan->this_token(), 111); //const expr doesn't match its type declaration
            scan->get_token();
         }
      }
      for (int i = 0; i<tokens.size(); i++)
      {
         id_tab_ent = id_tab->enter_id(tokens[i], ty, const_decl ? lille_kind::constant : lille_kind::variable, id_tab->scope(), 0, lille_type::type_unknown);
         if(const_decl = true)
            id_tab_ent->fix_const(i_const, r_const, s_const, b_const);
      }
   }

   scan->must_be(symbol::semicolon_sym);
   

   if(debugging)
      cout << "Parser: exiting decl()" << endl;
}

// Determine and return the type of a variable or constant.
lille_type parser::type()
{
   if(debugging)
      cout << "Parser: entering type()" << endl;

   // Check for different possible types and consume the corresponding symbols.
   if(scan->have(symbol::integer_sym))
   {
      scan->must_be(symbol::integer_sym);
      return lille_type::type_integer;
   }
   else if(scan->have(symbol::real_sym))
   {
      scan->must_be(symbol::real_sym);
      return lille_type::type_real;
   }
   else if(scan->have(symbol::string_sym))
   {
      scan->must_be(symbol::string_sym);
      return lille_type::type_string;
   }
   else if(scan->have(symbol::boolean_sym))
   {
      scan->must_be(symbol::boolean_sym);
      return lille_type::type_boolean;
   }
   else
      return lille_type::type_unknown; // Default to unknown type if none are found.
   /*
   else
   {
      error->flag(scan->this_token(), 129);
      scan->get_token();
   }
   */
   if(debugging)
      cout << "Parser: exiting type()" << endl;
}

// Process a list of parameters.
void parser::paramList()
{
   if(debugging)
      cout << "Parser: entering paramList()" << endl;
   // Process the first parameter.
   param();

   // Process the rest of the parameters separated by semicolons.
   while(scan->have(symbol::semicolon_sym))
   {
      scan->must_be(symbol::semicolon_sym);
      param();
   }
   if(debugging)
      cout << "Parser: exiting paramList()" << endl;
}

// Process a single parameter.
void parser::param()
{
   if(debugging)
      cout << "Parser: entering param()" << endl;
   // Process a list of identifiers representing parameters.
   identList();

   // Make sure a colon separates the parameter list from its type.
   scan->must_be(symbol::colon_sym);

   // Process the kind of parameter (value or reference) and its type.
   paramKind();
   type();

   if(debugging)
      cout << "Parser: exiting param()" << endl;
}

// Determine the kind of parameter (value or reference)
void parser::paramKind()
{
   if(debugging)
      cout << "Parser: entering paramKind()" << endl;

   // Check for the presence of 'value' or 'ref' symbols and consume them.
   if(scan->have(symbol::value_sym))
      scan->must_be(symbol::value_sym);
   else if(scan->have(symbol::ref_sym))
      scan->must_be(symbol::ref_sym);
   /*
   else
   {
      error->flag(scan->this_token(), 128); //paramKind must be a value or a ref symbol
   }
   */
   if(debugging)
      cout << "Parser: exiting paramKind()" << endl;
}

// Process a list of statements
void parser::statementList()
{
   if(debugging)
      cout << "Parser: entering statementList()" << endl;
   //Process the first statement.
   statement();

   // Process the rest of the statements separated by semicolons
   while(scan->have(symbol::semicolon_sym))
   {
      scan->must_be(symbol::semicolon_sym);
      statement();
   }
   if(debugging)
      cout << "Parser: exiting statementList()" << endl;
}

// Process a single statement.
void parser::statement()
{
   if(debugging)
      cout << "Parser: entering statement()" << endl; 

   // Check the type of statement and call the appropriate function.
   if(scan->have(symbol::identifier) || scan->have(symbol::exit_sym) || scan->have(symbol::return_sym) ||
      scan->have(symbol::read_sym) || scan->have(symbol::write_sym) || scan->have(symbol::writeln_sym) ||
      scan->have(symbol::null_sym))
   simpleStatement();
   else if(scan->have(symbol::if_sym) || scan->have(symbol::loop_sym) ||
           scan->have(symbol::for_sym) || scan->have(symbol::while_sym))
   {
      compoundStatement();
   }
   /*
   else
   {
      error->flag(scan->this_token(), 124); //statement must contain a simple or compound statement.
   }
   */
   if(debugging)
      cout << "Parser: exiting statement()" << endl;
}

// Process a simple statement.
void parser::simpleStatement()
{
   if(debugging)
      cout << "Parser: entering simpleStatement()" << endl;

   // Check the type of simple statement and process accordingly.
   if(scan->have(symbol::exit_sym))
   {
      // Handle EXIT statement.
      scan->must_be(symbol::exit_sym);
      if(scan->have(symbol::when_sym))
      {
         scan->must_be(symbol::when_sym);
         expr();
      }
   }
   else if(scan->have(symbol::return_sym))
   {
      // Handle RETURN statement
      scan->must_be(symbol::return_sym);
      if(scan->have(symbol::identifier) || scan->have(symbol::integer) || scan->have(symbol::real_num) || scan->have(symbol::strng)
         || scan->have(symbol::boolean_sym))
         expr();
   }
   else if(scan->have(symbol::read_sym))
   {
      // Handle READ statement
      bool parenOpen = false;
      scan->must_be(symbol::read_sym);
      if(scan->have(symbol::left_paren_sym))
      {
         parenOpen = true;
         scan->must_be(symbol::left_paren_sym);
      }
      identList();
      
      if (parenOpen)
         scan->must_be(symbol::right_paren_sym);
   }
   else if(scan->have(symbol::write_sym))
   {
      // Handle WRITE statement
      bool parenOpen = false;
      scan->must_be(symbol::write_sym);
      if(scan->have(symbol::left_paren_sym))
      {
         parenOpen = true;
         scan->must_be(symbol::left_paren_sym);
         if(scan->have(symbol::right_paren_sym))
         {
            error->flag(scan->this_token(),83);
         }
      }
      expr();

      while(scan->have(symbol::comma_sym))
      {
         scan->must_be(symbol::comma_sym);
         expr();
      }
      if(parenOpen)
         scan->must_be(symbol::right_paren_sym);
   }
   else if(scan->have(symbol::writeln_sym))
   {
      // Handle WRITELN statement.
      bool parenOpen = false;
      scan->must_be(symbol::writeln_sym);
      if(scan->have(symbol::left_paren_sym))
      {
         parenOpen = true;
         scan->must_be(symbol::left_paren_sym);
         if(scan->have(symbol::right_paren_sym))
         {
            error->flag(scan->this_token(), 83);
         }
      }
      expr();

      while(scan->have(symbol::comma_sym))
      {
         scan->must_be(symbol::comma_sym);
         expr();
      }
      if(parenOpen)
         scan->must_be(symbol::right_paren_sym);
   }
   else if(scan->have(symbol::null_sym))
      scan->must_be(symbol::null_sym);
   else if(scan->have(symbol::identifier))
   {
      // Handle assignment or function call.
      string current_entry_name = scan->get_current_identifier_name();
      curr_entry = id_tab->lookup(current_entry_name);

      /*
      if(curr_entry == NULL)
      {
         cout << "Error: Identifier not previously declared." << endl;
         //error->flag(scan->this_token(), 81); // Identifier not previously declared
      }
      

      if(curr_entry->tipe().is_type(lille_type::type_prog))
      {
         error->flag(scan->this_token(), 91);
      }
      */
      
      scan->must_be(symbol::identifier);
      if(scan->have(symbol::left_paren_sym))
      {
         // Process a function call.
         scan->must_be(symbol::left_paren_sym);
         expr();
         while(scan->have(symbol::comma_sym))
         {
            scan->must_be(symbol::comma_sym);
            expr();
         }
         scan->must_be(symbol::right_paren_sym);
      }
      else if(scan->have(symbol::becomes_sym))
      {  // Process assignment
         scan->must_be(symbol::becomes_sym);
         expr();
      }
      
   }
   if(debugging)
      cout << "Parser: exiting simpleStatement()" << endl;
}

// Process a compound statement based on its type.
void parser::compoundStatement()
{
   if(debugging)
      cout << "Parser: entering compoundStatement()" << endl;

   // Check the type of compound statement and process accordingly.
   if(scan->have(symbol::if_sym))
   {
      ifStatement();
   }
   else if(scan->have(symbol::loop_sym))
   {
      loopStatement();
   }
   else if(scan->have(symbol::for_sym))
   {
      forStatement();
   }
   else if(scan->have(symbol::while_sym))
   {
      whileStatement();
   }
   /*
   else
   {
      error->flag(scan->this_token(), 125);
      scan->get_token();
   }
   */
   if(debugging)
      cout << "Parser: exiting compoundStatement()" << endl;
}

// Process an IF statement.
void parser::ifStatement()
{
   if(debugging)
      cout << "Parser: entering ifStatement()" << endl;
   
   scan->must_be(symbol::if_sym);
   expr();
   scan->must_be(symbol::then_sym);
   statementList();

   while(scan->have(symbol::elsif_sym))
   {
      scan->must_be(symbol::elsif_sym);
      expr();
      scan->must_be(symbol::then_sym);
      statementList();
   }
   if(scan->have(symbol::else_sym))
   {
      scan->must_be(symbol::else_sym);
      statementList();
   }

   scan->must_be(symbol::end_sym);
   scan->must_be(symbol::if_sym);

   if(debugging)
      cout << "Parser: exiting ifStatement()" << endl;
}

// Process a while statement.
void parser::whileStatement()
{
   if(debugging)
      cout << "Parser: entering whileStatement()" << endl;
   scan->must_be(symbol::while_sym);
   expr();
   loopStatement();
   if(debugging)
      cout << "Parser: exiting whileStatement()" << endl;
}

// Process a for statement.
void parser::forStatement()
{
   if(debugging)
      cout << "Parser: entering forStatement()" << endl;
   scan->must_be(symbol::for_sym);
   id_tab->enter_scope();
   //id table entry for for loop identifier (i, j, k, etc)
   symbol* sym = new symbol(symbol::identifier);
   token* tok = new token(sym, 0, 0);
   tok->set_identifier_value(scan->get_current_identifier_name());
   id_table_entry* forEntry = id_tab->enter_id(tok, lille_type::type_integer, lille_kind::for_ident, id_tab->scope(), 0, lille_type::type_unknown);
   id_tab->add_table_entry(forEntry);

   scan->must_be(symbol::identifier);
   scan->must_be(symbol::in_sym);
   if(scan->have(symbol::reverse_sym))
      scan->must_be(symbol::reverse_sym);

   range();
   loopStatement();

   if(debugging)
      cout << "Parser: exiting forStatement()" << endl;
}

// Process a loop statement.
void parser::loopStatement()
{
   if(debugging)
      cout << "Parser: entering loopStatement()" << endl;
   scan->must_be(symbol::loop_sym);
   statementList();
   scan->must_be(symbol::end_sym);
   scan->must_be(symbol::loop_sym);
   id_tab->exit_scope();
   if(debugging)
      cout << "Parser: exiting loopStatement()" << endl;
}

// Process a range symbol.
void parser::range()
{
   if(debugging)
      cout << "Parser: entering range()" << endl;
   simpleExpr();
   scan->must_be(symbol::range_sym);
   simpleExpr();
   if(debugging)
      cout << "Parser: exiting range()" << endl;
}

// Process an expression.
void parser::expr()
{
   if(debugging)
      cout << "Parser: entering expr()" << endl;
   
   // Process the simple expression.
   simpleExpr();

   // Process a relational operator and another simple expression if present.
   if(scan->have(symbol::greater_than_sym) || scan->have(symbol::less_than_sym) ||
      scan->have(symbol::equals_sym) || scan->have(symbol::not_equals_sym) ||
      scan->have(symbol::less_or_equal_sym) || scan->have(symbol::greater_or_equal_sym))
   {
      relOp();
      simpleExpr();
   }

   // Process the 'IN' operator and a range if present.
   else if(scan->have(symbol::in_sym))
   {
      scan->must_be(symbol::in_sym);
      range();
   }
   /*
   else
   {
      error->flag(scan->this_token(), 127); //Invalid expr syntax
   }
   */
   if(debugging)
      cout << "Parser: exiting expr()" << endl;
}

// Process a simple expression.
void parser::simpleExpr()
{
   if(debugging)
      cout << "Parser: entering simpleExpr()" << endl;
   expr2();
   while(scan->have(symbol::ampersand_sym))
   {
      scan->must_be(symbol::ampersand_sym);
      expr2();
   }
   if(debugging)
      cout << "Parser: exiting simpleExpr()" << endl;
}

// Process a relational operator.
void parser::relOp()
{
   if(debugging)
      cout << "Parser: entering relOp()" << endl;

   // Check for the presence of differnt relational operators and consume them.
   if(scan->have(symbol::greater_than_sym))
      scan->must_be(symbol::greater_than_sym);
   else if(scan->have(symbol::less_than_sym))
      scan->must_be(symbol::less_than_sym);
   else if(scan->have(symbol::equals_sym))
      scan->must_be(symbol::equals_sym);
   else if(scan->have(symbol::not_equals_sym))
      scan->must_be(symbol::not_equals_sym);
   else if(scan->have(symbol::less_or_equal_sym))
      scan->must_be(symbol::less_or_equal_sym);
   else if(scan->have(symbol::greater_or_equal_sym))
      scan->must_be(symbol::greater_or_equal_sym);
   /*
   else
   {
      error->flag(scan->this_token(), 126);
      scan->get_token();
   }
   */
   if(debugging)
      cout << "Parser: exiting relOp()" << endl;
}

// Process the second level of expressions.
void parser::expr2()
{
   if(debugging)
      cout << "Parser: entering expr2()" << endl;
   // Process the first term.
   term();

   // Process additional terms connected by '+' or '-' or 'or' symbols.
   while(scan->have(symbol::plus_sym) || scan->have(symbol::minus_sym) || scan->have(symbol::or_sym))
   {
      // Check and consume the appropriate operator.
      if(scan->have(symbol::plus_sym))
      {
         scan->must_be(symbol::plus_sym);
      }
      else if(scan->have(symbol::minus_sym))
      {
         scan->must_be(symbol::minus_sym);
      }
      else if(scan->have(symbol::or_sym))
      {
         scan->must_be(symbol::or_sym);
      }

      // Process the next term.
      term();
   }
   if(debugging)
      cout << "Parser: exiting expr2()" << endl;
}

// Process a term in the expression.
void parser::term()
{
   if(debugging)
      cout << "Parser: entering term()" << endl;
   // Process the first factor.
   factor();

   // Process additional factors connected by '*' or '/' or 'and' symbols.
   while(scan->have(symbol::slash_sym) || scan->have(symbol::asterisk_sym) || scan->have(symbol::and_sym))
   {
      if(scan->have(symbol::asterisk_sym))
      {
        scan->must_be(symbol::asterisk_sym);
      }
      else if (scan->have(symbol::slash_sym))
      {
         scan->must_be(symbol::slash_sym);
      }
      else if (scan->have(symbol::and_sym))
      {
         scan->must_be(symbol::and_sym);
      }
      factor();
   }
   if(debugging)
      cout << "Parser: exiting term()" << endl;
}

// Process a factor in the expression.
void parser::factor()
{
   if(debugging)
      cout << "Parser: entering factor()" << endl;
   
   if(scan->have(symbol::plus_sym) || scan->have(symbol::minus_sym))
   {
      if(scan->have(symbol::plus_sym))
      {
         scan->must_be(symbol::plus_sym);
      }
      else if(scan->have(symbol::minus_sym))
      {
         scan->must_be(symbol::minus_sym);
      }
      primary();
   }
   else
   {
      primary();
      if(scan->have(symbol::power_sym))
      {
         scan->must_be(symbol::power_sym);
         primary();
      }
   }
   if(debugging)
      cout << "Parser: exiting factor()" << endl;
}

// Process a primary expression.
void parser::primary()
{
   if(debugging)
      cout << "Parser: entering primary()" << endl;
   // Check and process different types of primary expressions.
   if(scan->have(symbol::not_sym))
   {
      // Process the 'NOT' expression.
      scan->must_be(symbol::not_sym);
      expr();
   }
   else if(scan->have(symbol::odd_sym))
   {
      // Process the 'ODD' expression
      scan->must_be(symbol::odd_sym);
      expr();
   }
   else if(scan->have(symbol::left_paren_sym))
   {
      // Process an expression enclosed in parentheses.
      scan->must_be(symbol::left_paren_sym);
      simpleExpr();
      scan->must_be(symbol::right_paren_sym);
   }
   else if(scan->have(symbol::identifier))
   {
      // Process an identifier, check for a function call.
      scan->must_be(symbol::identifier);
      if(scan->have(symbol::left_paren_sym))
      {
         scan->must_be(symbol::left_paren_sym);
         expr();
         while(scan->have(symbol::comma_sym))
         {
            scan->must_be(symbol::comma_sym);
            expr();
         }
         scan->must_be(symbol::right_paren_sym);
      }
   }
   else if (scan->have(symbol::integer))
   {
      // Process an integer literal.
      scan->must_be(symbol::integer);
   }
   else if (scan->have(symbol::real_num))
   {
      // Process a real number literal.
      scan->must_be(symbol::real_num);
   }
   else if (scan->have(symbol::strng))
   {
      // Process a string literal.
      scan->must_be(symbol::strng);
   }
   else if (scan->have(symbol::true_sym))
   {
      // Process the 'TRUE' boolean literal.
      scan->must_be(symbol::true_sym);
   }
   else if (scan->have(symbol::false_sym))
   {
      // Process the 'FALSE' boolean literal.
      scan->must_be(symbol::false_sym);
   }
   
   /*
   else
   {
      error->flag(scan->this_token(), 127);
      scan->get_token();
   }
   */
   if(debugging)
      cout << "Parser: exiting primary()" << endl;
}

// Start the parser by getting the first token and calling prog().
void parser::program()
{
   if(debugging)
      cout << "Parser: entering program" << endl;

   scan->get_token();
   prog();

   if(debugging)
      cout << "Parser: exiting program" << endl;


}









