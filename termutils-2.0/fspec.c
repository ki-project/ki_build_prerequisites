/* Generated automatically from fspec.defs.  DO NOT EDIT. */

#include <tabs.h>

static int t_a[] = {1,10,16,36,72,0};
static int t_a2[] = {1,10,16,40,72,0};
static int t_c[] = {1,8,12,16,20,55,0};
static int t_c2[] = {1,6,10,14,49,0};
static int t_c3[] = {1,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62,67,0};
static int t_f[] = {1,7,11,15,19,23,0};
static int t_p[] = {1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,0};
static int t_s[] = {1,10,55,0};
static int t_u[] = {1,12,20,44,0};

struct fspec_table fspec_table[] = {
  {"a", t_a, "Assembler, IBM S/370, first format"},
  {"a2", t_a2, "Assembler, IBM S/370, second format"},
  {"c", t_c, "COBOL, normal format"},
  {"c2", t_c2, "COBOL, compact format"},
  {"c3", t_c3, "COBOL, compact format with more tab stops"},
  {"f", t_f, "FORTRAN"},
  {"p", t_p, "PL/I"},
  {"s", t_s, "SNOBOL"},
  {"u", t_u, "UNIVAC 1100 Assembler"},
  {0, 0}
};
