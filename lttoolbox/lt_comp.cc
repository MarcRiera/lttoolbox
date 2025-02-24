/*
 * Copyright (C) 2005 Universitat d'Alacant / Universidad de Alicante
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */
#include <lttoolbox/compiler.h>
#include <lttoolbox/att_compiler.h>
#include <lttoolbox/lt_locale.h>

#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>
#include <getopt.h>

using namespace std;

/*
 * Error function that does nothing so that when we fallback from
 * XML to AT&T, the user doesn't get a message unless it's really
 * invalid XML.
 */
void errorFunc(void *ctx, const char *msg, ...)
{
  return;
}

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << " v" << PACKAGE_VERSION <<": build a letter transducer from a dictionary" << endl;
    cout << "USAGE: " << basename(name) << " [-mavhlr] lr | rl dictionary_file output_file [acx_file]" << endl;
#if HAVE_GETOPT_LONG
    cout << "  -m, --keep-boundaries:     keep morpheme boundaries" << endl;
    cout << "  -v, --var:                 set language variant" << endl;
    cout << "  -a, --alt:                 set alternative (monodix)" << endl;
    cout << "  -l, --var-left:            set left language variant (bidix)" << endl;
    cout << "  -r, --var-right:           set right language variant (bidix)" << endl;
    cout << "  -H, --hfst:                expect HFST symbols" << endl;
#else
    cout << "  -m:     keep morpheme boundaries" << endl;
    cout << "  -v:     set language variant" << endl;
    cout << "  -a:     set alternative (monodix)" << endl;
    cout << "  -l:     set left language variant (bidix)" << endl;
    cout << "  -r:     set right language variant (bidix)" << endl;
    cout << "  -H:     expect HFST symbols" << endl;
#endif
    cout << "Modes:" << endl;
    cout << "  lr:     left-to-right compilation" << endl;
    cout << "  rl:     right-to-left compilation" << endl;
  }
  exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();

  char ttype = 'x';
  Compiler c;
  AttCompiler a;
  c.setKeepBoundaries(false);
  c.setVerbose(false);

#if HAVE_GETOPT_LONG
  int option_index=0;
#endif

  string vl;
  string vr;

  while (true) {
#if HAVE_GETOPT_LONG
    static struct option long_options[] =
    {
      {"alt",       required_argument, 0, 'a'},
      {"var",       required_argument, 0, 'v'},
      {"var-left",  required_argument, 0, 'l'},
      {"var-right", required_argument, 0, 'r'},
      {"keep-boundaries",      no_argument,       0, 'm'},
      {"hfst",      no_argument,       0, 'H'},
      {"help",      no_argument,       0, 'h'},
      {"verbose",   no_argument,       0, 'V'},
      {0, 0, 0, 0}
    };

    int cnt=getopt_long(argc, argv, "a:v:l:r:mHhV", long_options, &option_index);
#else
    int cnt=getopt(argc, argv, "a:v:l:r:mHhV");
#endif
    if (cnt==-1)
      break;

    switch (cnt)
    {
      case 'a':
        c.setAltValue(to_ustring(optarg));
        break;

      case 'v':
        c.setVariantValue(to_ustring(optarg));
        break;

      case 'l':
        vl = optarg;
        c.setVariantLeftValue(to_ustring(optarg));
        break;

      case 'r':
        vr = optarg;
        c.setVariantRightValue(to_ustring(optarg));
        break;

      case 'm':
        c.setKeepBoundaries(true);
        break;

      case 'H':
        a.setHfstSymbols(true);
        break;

      case 'V':
        c.setVerbose(true);
        break;

      case 'h':
      default:
        endProgram(argv[0]);
        break;
    }
  }

  string opc;
  string infile;
  string outfile;
  string acxfile;

  switch(argc - optind + 1)
  {
    case 5:
      opc = argv[argc-4];
      infile = argv[argc-3];
      outfile = argv[argc-2];
      acxfile = argv[argc-1];
      break;

    case 4:
      opc = argv[argc-3];
      infile = argv[argc-2];
      outfile = argv[argc-1];
      break;

    default:
      endProgram(argv[0]);
      break;
  }

  xmlTextReaderPtr reader;
  reader = xmlReaderForFile(infile.c_str(), NULL, 0);
  xmlGenericErrorFunc handler = (xmlGenericErrorFunc)errorFunc;
  initGenericErrorDefaultFunc(&handler);
  if(reader != NULL)
  {
    int ret = xmlTextReaderRead(reader);
    if(ret != 1)
    {
      ttype = 'a';
    }
    xmlFreeTextReader(reader);
    xmlCleanupParser();
  }
  else
  {
    cerr << "Error: Cannot not open file '" << infile << "'." << endl << endl;
    exit(EXIT_FAILURE);
  }
  initGenericErrorDefaultFunc(NULL);


  if(opc == "lr")
  {
    if(vr == "" && vl != "")
    {
      cout << "Error: -l specified, but mode is lr" << endl;
      endProgram(argv[0]);
    }
    if(ttype == 'a')
    {
      a.parse(infile, false);
    }
    else
    {
      if(acxfile != "")
      {
        c.parseACX(acxfile, Compiler::COMPILER_RESTRICTION_LR_VAL);
      }
      c.parse(infile, Compiler::COMPILER_RESTRICTION_LR_VAL);
    }
  }
  else if(opc == "rl")
  {
    if(vl == "" && vr != "")
    {
      cout << "Error: -r specified, but mode is rl" << endl;
      endProgram(argv[0]);
    }
    if(ttype == 'a')
    {
      a.parse(infile, true);
    }
    else
    {
      c.parse(infile, Compiler::COMPILER_RESTRICTION_RL_VAL);
    }
  }
  else
  {
    endProgram(argv[0]);
  }

  FILE *output = fopen(outfile.c_str(), "wb");
  if(!output)
  {
    cerr << "Error: Cannot open file '" << outfile << "'." << endl;
    exit(EXIT_FAILURE);
  }
  if(ttype == 'a')
  {
    a.write(output);
  }
  else
  {
    c.write(output);
  }
  fclose(output);
}
