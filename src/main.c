/***********************************************************
* This file is part of glyr
* + a commnadline tool and library to download various sort of musicrelated metadata.
* + Copyright (C) [2011]  [Christopher Pahl]
* + Hosted at: https://github.com/sahib/glyr
*
* glyr is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* glyr is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with glyr. If not, see <http://www.gnu.org/licenses/>.
**************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>

#include "../lib/glyr.h"
#include "../lib/translate.h"

// also includes glyr's stringlib
// you shouldnt do this yourself, as I don't guarantee to keep this stable.
// implementing functions twice is silly though, so glyrc is an exception
// (in this case at least )
#include "../lib/stringlib.h"

// For glyr_message. This time you *never* should use this
#include "../lib/core.h"

#include <dirent.h>
#include <sys/types.h>

bool update = false;
const char * default_path = ".";
const char * exec_on_call = NULL;
const char ** write_arg   = NULL;

//* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static void print_version(GlyQuery * s)
{
        glyr_message(-1,s,stdout, C_"%s\n\n"C_,Gly_version());
        glyr_message(-1,s,stderr, C_"This is still beta software, expect quite a lot bugs.\n");
        glyr_message(-1,s,stderr, C_"Email bugs to <sahib@online.de> or use the bugtracker\n"
	                          C_"at https://github.com/sahib/glyr/issues - Thank you! \n");

        exit(0);
}

//* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static void free_name_list(const char ** ptr) {
	if(ptr != NULL) {
		size_t i = 0;
		while(ptr[i] != NULL) {
			free((char*)ptr[i++]);
		}
		free(ptr);
	}
}


/* --------------------------------------------------------- */

#define _S "\t"
static void list_provider_at_id(int id, int min_align, GlyQuery * s)
{
	const char **   cp_name = GlyPlug_get_name_by_id(id);
	const char *    cp_keys = GlyPlug_get_key_by_id(id);
	unsigned char * cp_gids = GlyPlug_get_gid_by_id(id);

        s->color_output = false;

        if(cp_name != NULL) {
                int i = 0;
                for(i = 0; cp_name[i] != NULL; i++) {
                        int align = min_align - strlen(cp_name[i]);
                        int a = 0;
                        glyr_message(-1,s,stdout,C_G _S"%s"C_,cp_name[i]);
                        for(a = 0; a < align; a++)
                                glyr_message(-1,s,stdout," ");

                        glyr_message(-1,s,stdout,C_" ["C_R"%c"C_"]%s",cp_keys[i], (id == GET_UNSURE) ? "" : " in groups "C_Y"all"C_);
                        int b = 1, j = 0;
                        for(b = 1, j = 0; id != GET_UNSURE &&  b <= GRP_ALL; j++) {
                                if(b & cp_gids[i]) {
                                        glyr_message(-1,s,stdout,C_","C_Y"%s",Gly_groupname_by_id(b));
                                }
                                b <<= 1;
                        }
                        glyr_message(-1,s,stdout,"\n");
                }
        }

	free_name_list(cp_name);

	if(cp_keys) {
	    free((char*)cp_keys);
	}

	if(cp_gids) {
	    free(cp_gids);
	}
}

//* ------------------------------------------------------- */

static void sig_handler(int signal)
{
        switch(signal) {
        case SIGINT:
                glyr_message(-1,NULL,stderr,C_" Received keyboard interrupt!\n");
                break;
        case SIGSEGV : /* sigh */
                glyr_message(-1,NULL,stderr,C_R"\nFATAL: "C_"libglyr crashed due to a Segmentation fault.\n");
                glyr_message(-1,NULL,stderr,C_"       This is entirely the fault of the libglyr developers. Yes, we failed. Sorry. Now what to do:\n");
                glyr_message(-1,NULL,stderr,C_"       It would be just natural to blame us now, so just visit <https://github.com/sahib/glyr/issues>\n");
                glyr_message(-1,NULL,stderr,C_"       and throw hard words like 'backtrace', 'bug report' or even the '$(command I issued' at them).\n");
                glyr_message(-1,NULL,stderr,C_"       The libglyr developers will try to fix it as soon as possible so you stop pulling their hair.\n");

                glyr_message(-1,NULL,stderr,C_"\n(Thanks, and Sorry for any bad feelings.)\n");
                break;
        }
        exit(-1);
}


//* ------------------------------------------------------- */
// -------------------------------------------------------- //
/* -------------------------------------------------------- */

static bool set_get_type(GlyQuery * s, const char * arg)
{
        bool result = false;

        if(!arg) {
                return true;
        }
        // get list of avaliable commands
	const char **   cp_name = GlyPlug_get_name_by_id(GET_UNSURE);
	const char *    cp_keys = GlyPlug_get_key_by_id(GET_UNSURE);
	unsigned char * cp_gids = GlyPlug_get_gid_by_id(GET_UNSURE);

        if(cp_name && cp_keys && cp_gids) {
                int i = 0;
                for(; cp_name[i]; i++) {
                        if(!strcmp(arg,cp_name[i]) || *arg == cp_keys[i]) {
                                GlyOpt_type(s,cp_gids[i]);
                                result = true;
                        }
                }

		if(result == false) {
			glyr_message(2,s,stderr,"Sorry, I don't know of a getter called '%s'...\n\n",arg);
			glyr_message(2,s,stderr,"Avaliable getters are: \n");
			glyr_message(2,s,stderr,"---------------------- \n");

			list_provider_at_id(GET_UNSURE,7,s);

			bool dym = false;
			for(i = 0; cp_name[i]; i++) {
				if(levenshtein_strcmp(arg,cp_name[i]) <= 4) {
					if(!dym) glyr_message(2,s,stderr,"\nPerhaps you've meant");
					glyr_message(2,s,stderr,"%s '%s'",dym?" or":" ",cp_name[i]);
					dym=true;
				}
			}
			if(dym) glyr_message(2,s,stderr,"?\n");
		}
		
		free_name_list(cp_name);
		free((char*)cp_keys);
		free(cp_gids);
        }
        return result;
}

//* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static void search_similiar_providers(const char * providers, GlyQuery * s)
{
	const char **   cp_name = GlyPlug_get_name_by_id(GET_UNSURE);
	const char *    cp_keys = GlyPlug_get_key_by_id(GET_UNSURE);

        if(cp_name && cp_keys) {
                size_t length = strlen(providers);
                size_t offset = 0;
                char * name = NULL;

                while( (name = get_next_word(providers,DEFAULT_FROM_ARGUMENT_DELIM,&offset,length))) {
                        int j;

                        bool f = false;
                        for(j = 0; cp_name[j]; j++) {
                                if(!strcasecmp(cp_name[j],name) || (cp_keys[j] == *name)) {
                                        f = true;
                                        break;
                                }
                        }
                        if(!f) {
                                bool dym = true;
                                for(j = 0; cp_name[j]; j++) {
                                        if(levenshtein_strcmp(name,cp_name[j]) < 3) {
                                                if(dym) {
                                                        glyr_message(2,s,stderr,C_R"* "C_"Did you mean");
                                                }

                                                glyr_message(2,s,stderr,"%s "C_G"%s"C_" ",dym?"":" or",cp_name[j]);

                                                dym = false;
                                        }
                                }
                                if(!dym) glyr_message(2,s,stderr,"?\n",name);
                        }
                        free(name);
                        name=NULL;
                }

                glyr_message(2,s,stderr,C_R"* "C_"Must be one of:\n");
                list_provider_at_id(s->type,15,s);
                glyr_message(2,s,stderr,"\n");

		free_name_list(cp_name);
		free((char*) cp_keys);
        }
}

//* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */


static void suggest_other_options(int m, int argc, char * const * argv, int at, struct option * long_options, GlyQuery * s)
{
        bool dym = false;

        char * argument = argv[at];
        while(*argument && *argument == '-')
                argument++;

        glyr_message(1,s,stderr,C_R"*"C_" Unknown option or missing argument: %s\n",argv[at]);
        if(argument && *argument) {
                int i = 0;
                for(i = 0; i < m && long_options[i].name; i++) {
                        if(levenshtein_strcmp(argument,long_options[i].name) < 3 || *(argument) == long_options[i].val) {
                                if(!dym) {
                                        glyr_message(1,s,stderr,C_G"*"C_" Did you mean");
                                }

                                glyr_message(1,s,stderr,C_G"%s'--%s' (-%c) "C_,!dym?" ":"or",long_options[i].name,long_options[i].val);
                                dym=true;
                        }
                }
        }
        if(dym) {
                glyr_message(1,s,stderr,"?\n");
        }
        if(optind == argc-1) {
                glyr_message(1,s,stderr,"\n");
        }
}

//* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static bool check_if_dir(const char * path)
{
        if(strcasecmp(optarg,"stdout") && strcasecmp(optarg,"stderr") && strcasecmp(optarg,"null")) {
                DIR * dummy = opendir(path);
                if(dummy) {
                        closedir(dummy);
                        return true;
                } else {
                        return false;
                }
        }
        return true;
}

/* --------------------------------------------------------- */

void help_short(GlyQuery * s)
{
        glyr_message(-1,s,stderr,"Usage: glyrc [GETTER] (options)\n\nwhere [GETTER] must be one of:\n");

        list_provider_at_id(GET_UNSURE,10,s);

        glyr_message(-1,s,stderr,"\nIf you're viewing this helptext the first time,\n"
                     "you probably want to view --usage, which has more details,\n"
                     "and also offers examples and a full list of all builtin providers.\n"
                    );


// spacer
#define IN "    "
        glyr_message(-1,s,stderr,"\n\nOPTIONS:\n"
                     IN"-f --from  <s>        Providers from where to get metadata. Refer to the list at the end of this text.\n"
                     IN"-w --write <d>        Write metadata to dir <d>, special values stdout, stderr and null are supported\n"
                     IN"-p --parallel <i>     Integer. Define the number of downloads that may be performed in parallel.\n"
                     IN"-r --redirects        Integer. Define the number of redirects that are allowed.\n"
                     IN"-m --timeout          Integer. Define the maximum number in seconds after which a download is cancelled.\n"
                     IN"-x --plugmax          Integer. Maximum number od download a plugin may deliever. Use to make results more vary.\n"
                     IN"-v --verbosity        Integer. Set verbosity from 0 to 4. See --usage for details.\n"
                     IN"-u --update           Also download metadata if files are already in path (given by -w or '.')\n"
                     IN"-U --skip-update      Do not download data if already present.\n"
                     IN"-h --help             This text you unlucky wanderer are viewing.\n"
                     IN"-H --usage            A more detailed version of this text.\n"
                     IN"-V --version          Print the version string.\n"
                     IN"-c --color            Enable colored output (Unix only)\n"
                     IN"-C --skip-color       Disable colored output. (Unix only)\n"
                     IN"-d --download         Download Images.\n"
                     IN"-D --skip-download    Don't download images, but return the URLs to them (act like a search engine)\n"
                     IN"-g --groups           Enable grouped download (Slower but more accurate, as quality > speed)\n"
                     IN"-G --skip-groups      Query all providers at once. (Faster but may deliever weird results)\n"
                     IN"-y --twincheck        Check for duplicate URLs, yes by default\n"
                     IN"-y --skip-twincheck   Disable URLduplicate check.\n"
                     IN"-a --artist           Artist name (Used by all plugins)\n"
                     IN"-b --album            Album name (Used by cover,review,lyrics)\n"
                     IN"-t --title            Songname (used mainly by lyrics)\n"
                     IN"-e --maxsize          (cover only) The maximum size a cover may have.\n"
                     IN"-i --minsize          (cover only) The minimum size a cover may have.\n"
                     IN"-n --number           Download max. <n> items. Amount of actual downloaded items may be less.\n"
                     IN"-o --prefer           (images only) Only allow certain formats (Default: %s)\n"
                     IN"-t --lang             Language settings. Used by a few getters to deliever localized data. Given in ISO 639-1 codes\n"
                     IN"-f --fuzzyness        Set treshold for level of Levenshtein algorithm.\n"
                     IN"-j --callback         Set a bash command to be executed when a item is finished downloading;\n"
		     IN"-k --proxy	      Set the proxy to use in the form of [protocol://][user:pass@]yourproxy.domain[:port]\n"
                     IN"                      The special string <path> is expanded with the actual path to the data.\n",
                     DEFAULT_FORMATS
                    );

        glyr_message(-1,s,stdout,C_"\nAUTHOR: (C) Christopher Pahl - 2011, <sahib@online.de>\n%s\n",Gly_version());
        exit(EXIT_FAILURE);
}


static void usage(GlyQuery * s)
{
        glyr_message(-1,s,stderr,"Usage: glyrc [GETTER] (options)\n\nwhere [GETTER] must be one of:\n");
        list_provider_at_id(GET_UNSURE,10,s);
        glyr_message(-1,s,stderr,C_B"\nGENERAL OPTIONS\n"C_
                     IN C_C"-f --from  <providerstring>\n"C_
                     IN IN"Use this to define what providers you want to use.\n"
                     IN IN"Every provider has a name and a key which is merely a shortcut for the name.\n"
                     IN IN"Specify all providers in a semicolon seperated list.\n"
                     IN IN"See the list down for a complete list of all providers.\n"
                     IN IN"Example:\n"
                     IN IN"  \"amazon;google\"\n"
                     IN IN"  \"a;g\" - same, just with keys\n\n"
                     IN IN"You can also prepend each word with a '+' or a '-' ('+' is assumend without),\n"
                     IN IN"which will add or remove this provider from the list respectively.\n"
                     IN IN"Additionally you may use the predefined groups 'safe','unsafe','fast','slow','special'.\n\n"
                     IN IN"Example:\n"
                     IN IN"  \"+fast;-amazon\" which will enable last.fm and lyricswiki.\n\n"
                     IN C_C"-w --write <dir>\n"C_
                     IN IN"The directory to write files too, filenames are built like this:\n"
                     IN IN"  $dir/$artist_$album_cover_$num.jpg\n"
                     IN IN"  $dir/$artist_photo_$num.jpg\n"
                     IN IN"  $dir/$artist_$title_lyrics_$num.txt\n"
                     IN IN"  $dir/$artist_$album_review_$num.txt\n"
                     IN IN"  $dir/$artist_url_$num.txt\n"
                     IN IN"  $dir/$artist_tag_$num.txt\n"
                     IN IN"  $dir/$artist_$album_tracktitle_$num.txt\n"
                     IN IN"  $dir/$artist_albumtitle_$num.txt\n"
                     IN IN"  $dir/$artist_similiar_$num.txt\n"
                     IN IN"  $dir/$artist_ainfo_$num.txt\n"
                     IN IN"Note: This might change again in future releases.\n"
                     IN C_C"-x --plugmax\n"C_
                     IN IN"Maximum number of items a provider may deliever.\n"
                     IN IN"Use this to scatter the searchresults, or set it\n"
                     IN IN"to -1 if you don't care (=> default).\n\n"
                     IN C_C"-d --download\n"C_
                     IN IN"For image getters only.\n"
                     IN IN"If set to true images are also coviniently downloaded and returned.\n"
                     IN IN"otherwise, just the URL is returned for your own use. (specify -D)\n\n"
                     IN IN"Default to to 'true', 'false' would be a bit more searchengine like.\n"
                     IN C_C"-g --groups\n"C_
                     IN IN"If set false (by -G), this will disable the grouping of providers.\n"
                     IN IN"By default providers are grouped in categories like 'safe','unsafe','fast' etc., which\n"
                     IN IN"are queried in parallel, so the 'best' providers are queried first.\n"
                     IN IN"Disabling this behaviour will result in increasing speed, but as a result the searchresults\n"
                     IN IN"won't be sorted by quality, as it is normally the case.\n\n"
                     IN IN"Use with care.\n\n"
                     IN C_C"-y --twincheck\n"C_
                     IN IN"Will check for duplicate items if given.\n"
                     IN C_C"-j --callback\n"
                     IN IN"Set a bash command to be executed when a item is finished downloading;\n"
                     C_B"\nLIBCURL OPTIONS\n"C_
                     IN C_C"-p --parallel <int>\n"C_
                     IN IN"Integer. Define the number of downloads that may be performed in parallel.\n"
                     IN C_C"-r --redirects <int>\n"C_
                     IN IN"Integer. Define the number of redirects that are allowed.\n"
		     IN C_C"-k --proxy <string>\n"C_ 
		     IN IN "Set the proxy you want to use, none by default.\n"
		     IN IN"Syntax: http://user:pass@yourproxy.domain:port\n"
		     IN IN"Example: \"http://jott:root@Proxy.fh-hof.de:3128\"\n"
                     IN C_C"-m --timeout\n"C_
                     IN IN"Integer. Define the maximum number in seconds after which a download is cancelled.\n"
                     C_B"\nPLUGIN SPECIFIC OPTIONS\n"C_
                     IN C_C"-a --artist\n"C_
                     IN IN"The artist option is required for ALL getters.\n"
                     IN C_C"-b --album\n"C_
                     IN IN"Required for the following getters:\n"
                     IN IN IN" - albumlist\n"
                     IN IN IN" - cover\n"
                     IN IN IN" - review\n"
                     IN IN IN" - tracklist\n"
                     IN IN"Optional for those:\n"
                     IN IN IN" - tags\n"
                     IN IN IN" - relations\n"
                     IN IN IN" - lyrics (might be used by a few providers)\n"
                     IN C_C"-t --title\n"C_
                     IN IN"Set the songtitle.\n\n"
                     IN IN"Required for:\n"
                     IN IN"- lyrics\n"
                     IN IN"Optional for:\n"
                     IN IN"- tags\n"
                     IN IN"- relations\n"
                     IN C_C"-e --maxsize\n"C_
                     IN IN"The maximum size a cover may have.\n"
                     IN IN"As cover have mostly a 1:1 aspect ratio only one size is given with 'size'.\n"
                     IN IN"This has no effect if specified with 'photos'.\n"
                     IN C_C"-i --minsize\n"C_
                     IN IN"Same as --maxsize, just for the minimum size.\n"
                     IN C_C"-n --number\n"C_
                     IN IN"How many items to search for (at least 1)\n"
                     IN IN"This is mostly not the number of items actually returned then,\n"
                     IN IN"because libglyr is not able to find 300 songtexts of the same song,\n"
                     IN IN"or glyr filters duplicate items before returning.\n"
                     IN C_C"-o --prefer\n"C_
                     IN IN"Awaits a string with a semicolon seperated list of allowed formats.\n"
                     IN IN"The case of the format is ignored.\n\n"
                     IN IN"Example:\n"
                     IN IN"    \"png;jpg;jpeg\" would allow png and jpeg.\n\n"
                     IN IN"You can also specify \"all\", which disables this check.\n"
                     IN C_C"-t --lang\n"C_
                     IN IN"The language used for providers with multilingual content.\n"
                     IN IN"It is given in ISO-639-1 codes, i.e 'de','en','fr' etc.\n"
                     IN IN"List of providers recognizing this option:\n"
                     IN IN"  * cover/amazon (which amazon server to query)\n"
                     IN IN"  * cover/google (which google server to query)\n"
                     IN IN"  * ainfo/lastfm (the language the biography shall be in)\n"
                     IN IN"  * (this list should get longer in future releases)\n"
                     IN IN"(Use only these providers if you really want ONLY localized content)\n"
                     IN IN"By default all search results are in english.\n"
                     IN C_C"-f --fuzzyness\n"C_
                     IN IN"Set the maximum amount of inserts, edits and substitutions, a search results\n"
                     IN IN"may differ from the artist and/or album and/or title.\n"
                     IN IN"The difference between two strings is measured as the 'Levenshtein distance',\n"
                     IN IN"i.e, the total amount of inserts,edits and substitutes needed to convert string a to b.\n"
                     IN IN"Example:\n"
                     IN IN"  \"Equilibrium\" <=> \"Aqilibriums\" => Distance=3\n"
                     IN IN"  With a fuzzyness of 3 this would pass the check, with 2 it won't.\n"
                     IN IN" Higher values mean more search results, but more inaccuracy.\n"
                     IN IN" Default is 4.\n"
                     C_B"\nMISC OPTIONS\n"C_
                     IN C_C"-h --help\n"C_
                     IN IN"A shorter version of this text.\n"
                     IN C_C"-H --usage\n"C_
                     IN IN"The text just jumping through your eyes in search for meaning.\n"
                     IN C_C"-V --version\n"C_
                     IN IN"Print the version string, it's also at the end of this text.\n"
                     IN C_C"-c --color\n"C_
                     IN IN"Maximum number of items a provider may deliever.\n"
                     IN IN"Use this to scatter the searchresults, or set it\n"
                     IN IN"to -1 if you don't care (=> default).\n"
                     IN C_C"-v --verbosity\n"C_
                     IN IN"Set the verbosity level from 0-4, where:\n"
                     IN IN"0) nothing but fatal errors.\n"
                     IN IN"1) warnings and important notes.\n"
                     IN IN"2) normal, additional information what libglyr does.\n"
                     IN IN"3) basic debug output.\n"
                     IN IN"4) libcurl debug output.\n"
                     "\n"
                     IN C_C"-u --update\n"C_
                     IN IN"If files are already present in the path given by --write (or '.' if none given),\n"
                     IN IN"no searching is performed by default. Use this flag to disable this behaviour.\n"
                     "\n\n"
                     "The boolean options -u,-c,-y,-g,-d  have an uppercase version,\nwhich is inversing it's effect. The long names are prepended by '--skip'\n\n"
                    );



        glyr_message(-1,s,stderr, "A list of providers you can specify with --from folows:\n");
        glyr_message(-1,s,stderr,"\n"IN C_"cover:\n");
        list_provider_at_id(GET_COVERART,13,s);
        glyr_message(-1,s,stderr,"\n"IN C_"lyrics:\n");
        list_provider_at_id(GET_LYRICS,13,s);
        glyr_message(-1,s,stderr,"\n"IN C_"photos:\n");
        list_provider_at_id(GET_ARTIST_PHOTOS,13,s);
        glyr_message(-1,s,stderr,"\n"IN C_"review:\n");
        list_provider_at_id(GET_ALBUM_REVIEW,13,s);
        glyr_message(-1,s,stderr,"\n"IN C_"similiar:\n");
        list_provider_at_id(GET_SIMILIAR_ARTISTS,13,s);
        glyr_message(-1,s,stderr,"\n"IN C_"ainfo:\n");
        list_provider_at_id(GET_ARTISTBIO,13,s);
        glyr_message(-1,s,stderr,"\n"IN C_"tracklist:\n");
        list_provider_at_id(GET_TRACKLIST,13,s);
        glyr_message(-1,s,stderr,"\n"IN C_"albumlist:\n");
        list_provider_at_id(GET_ALBUMLIST,13,s);
        glyr_message(-1,s,stderr,"\n"IN C_"tags\n");
        list_provider_at_id(GET_TAGS,13,s);
        glyr_message(-1,s,stderr,"\n"IN C_"relations\n");
        list_provider_at_id(GET_RELATIONS,13,s);


        glyr_message(-1,s,stderr,"\n\n");
        print_version(s);
}

/* --------------------------------------------------------- */

static const char ** parse_commandline_general(int argc, char * const * argv, GlyQuery * glyrs)
{
        int c;
        const char ** loc_write_arg = NULL;
        bool haswrite_arg = false;

        static struct option long_options[] = {
                {"from",          required_argument, 0, 'f'},
                {"write",         required_argument, 0, 'w'},
                {"parallel",      required_argument, 0, 'p'},
                {"redirects",     required_argument, 0, 'r'},
                {"timeout",       required_argument, 0, 'm'},
                {"plugmax",       required_argument, 0, 'x'},
                {"verbosity",     required_argument, 0, 'v'},
                {"update",        no_argument,       0, 'u'},
                {"no-update",     no_argument,       0, 'U'},
                {"help",          no_argument,       0, 'h'},
                {"usage",         no_argument,       0, 'H'},
                {"version",       no_argument,       0, 'V'},
                {"color",         no_argument,       0, 'c'},
                {"no-color",      no_argument,       0, 'C'},
                {"download",      no_argument,       0, 'd'},
                {"no-download",   no_argument,       0, 'D'},
                {"groups",        no_argument,       0, 'g'},
                {"no-groups",     no_argument,       0, 'G'},
                {"twincheck",     no_argument,       0, 'y'},
                {"no-twincheck",  no_argument,       0, 'Y'},
                // ------- plugin specific ------- //
                {"artist",        required_argument, 0, 'a'},
                {"album",         required_argument, 0, 'b'},
                {"title",         required_argument, 0, 't'},
                {"minsize",       required_argument, 0, 'i'},
                {"maxsize",       required_argument, 0, 'e'},
                {"number",        required_argument, 0, 'n'},
                {"lang",          required_argument, 0, 'l'},
                {"fuzzyness",     required_argument, 0, 'z'},
                {"prefer",        required_argument, 0, 'o'},
                {"callback",	  required_argument, 0, 'j'},
		{"targetlang",    required_argument, 0, 's'},
		{"sourcelang",    required_argument, 0, 'q'},
		{"proxy",	  required_argument, 0, 'k'},
                {0,               0,                 0, '0'}
        };

        while (true) {
                int option_index = 0;
                c = getopt_long_only(argc, argv, "uUVhHcCyYdDgGf:w:p:r:m:x:v:a:b:t:i:e:n:l:z:o:j:s:q:k:",long_options, &option_index);

                // own error report
                opterr = 0;

                if (c == -1)
                        break;

                switch (c) {
                case 'w': {
                        size_t length = strlen(optarg);
                        size_t offset = 0;
                        size_t elemct = 0;

                        char * c_arg = NULL;
                        while( (c_arg = get_next_word(optarg,DEFAULT_FROM_ARGUMENT_DELIM,&offset,length)) != NULL) {
                                const char * element = NULL;
                                if(*c_arg && strcasecmp(c_arg,"stdout") && strcasecmp(c_arg,"stderr") && strcasecmp(c_arg,"null")) {
                                        if(check_if_dir(c_arg) == false) {
                                                glyr_message(1,glyrs,stderr,C_R"*"C_" %s is not a valid directory.\n",c_arg);
                                        } else {
                                                size_t word_len = strlen(c_arg);
                                                if(c_arg[word_len-1] == '/')
                                                        c_arg[word_len-1] = '\0';

                                                element = c_arg;
                                        }
                                } else if(*c_arg) {
                                        element = c_arg;
                                }

                                if(element) {
                                        loc_write_arg = realloc(loc_write_arg, (elemct+2) * sizeof(char*));
                                        loc_write_arg[  elemct] = strdup(element);
                                        loc_write_arg[++elemct] = NULL;
                                        haswrite_arg = true;
                                }
                                free(c_arg);
                                c_arg=NULL;
                        }
                        if(elemct == 0) {
                                glyr_message(1,glyrs,stderr,C_R"*"C_" you should give at least one valid dir to --w!\n");
                                glyr_message(1,glyrs,stderr,C_R"*"C_" Will default to the current working directory.\n");
                                loc_write_arg = NULL;
                        }
                        break;
                }
                case 'u':
                        update = true;
                        break;
                case 'U':
                        update = false;
                        break;
                case 'y':
                        GlyOpt_duplcheck(glyrs,true);
                        break;
                case 'Y':
                        GlyOpt_duplcheck(glyrs,false);
                        break;
                case 'g':
                        GlyOpt_groupedDL(glyrs,true);
                        break;
                case 'G':
                        GlyOpt_groupedDL(glyrs,false);
                        break;
                case 'f':
                        if(GlyOpt_from(glyrs,optarg) != GLYRE_OK)
                                search_similiar_providers(optarg,glyrs);
                        break;
                case 'v':
                        GlyOpt_verbosity(glyrs,atoi(optarg));
                        break;
                case 'p':
                        GlyOpt_parallel(glyrs,atoi(optarg));
                        break;
                case 'r':
                        GlyOpt_redirects(glyrs,atoi(optarg));
                        break;
                case 'm':
                        GlyOpt_timeout(glyrs,atoi(optarg));
                        break;
                case 'x':
                        GlyOpt_plugmax(glyrs,atoi(optarg));
                        break;
                case 'V':
                        print_version(glyrs);
                        break;
                case 'h':
                        help_short(glyrs);
                        break;
                case 'H':
                        usage(glyrs);
                        break;
                case 'c':
                        GlyOpt_color(glyrs,true);
                        break;
                case 'C':
                        GlyOpt_color(glyrs,false);
                        break;
                case 'a':
                        GlyOpt_artist(glyrs,optarg);
                        break;
                case 'b':
                        GlyOpt_album(glyrs,optarg);
                        break;
                case 't':
                        GlyOpt_title(glyrs,optarg);
                        break;
                case 'i':
                        GlyOpt_cminsize(glyrs,atoi(optarg));
                        break;
                case 'e':
                        GlyOpt_cmaxsize(glyrs,atoi(optarg));
                        break;
                case 'n':
                        GlyOpt_number(glyrs,atoi(optarg));
                        break;
                case 'd':
                        GlyOpt_download(glyrs,true);
                        break;
                case 'D':
                        GlyOpt_download(glyrs,false);
                        break;
                case 'l':
                        GlyOpt_lang(glyrs,optarg);
                        break;
                case 'z':
                        GlyOpt_fuzzyness(glyrs,atoi(optarg));
                        break;
                case 'o':
                        GlyOpt_formats(glyrs,optarg);
                        break;
                case 'j':
                        exec_on_call = optarg;
                        break;
		case 'q':
			GlyOpt_gtrans_source_lang(glyrs,optarg);
			break;
		case 'k':
			GlyOpt_proxy(glyrs,optarg);
			break;
		case 's':
			GlyOpt_gtrans_target_lang(glyrs,optarg);
			break;
                case '?':
                        suggest_other_options(sizeof(long_options) / sizeof(struct option), argc, argv, optind-1, long_options,glyrs);
                        break;
                }
        }

        if (optind < argc) {
                while (optind < argc) {
                        suggest_other_options(sizeof(long_options) / sizeof(struct option), argc, argv, optind, long_options, glyrs);
                        optind++;
                }
        }
        if(haswrite_arg) return loc_write_arg;

        return NULL;
}

//* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

char * correct_path(const char * path)
{
        char * r = NULL;
        if(path) {
                char * no_slash = strreplace(path,"/","+");
                if(no_slash) {
                        char * no_spaces = strreplace(no_slash," ","+");
                        if(no_spaces) {
                                r = no_spaces;
                        }
                        free(no_slash);
                        no_slash=NULL;
                }
        }
        return r;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_covers(GlyQuery * s, const char * save_dir, int i)
{
        char * good_artist = correct_path(s->artist);
        char * good_album  = correct_path(s->album );
        char * good_path   =  strdup_printf("%s/%s_%s_cover_%d.jpg",save_dir, good_artist,good_album,i);

        if(good_album)
                free(good_album);
        if(good_artist)
                free(good_artist);

        return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_lyrics(GlyQuery * s, const char * save_dir, int i)
{
        char * good_artist = correct_path(s->artist);
        char * good_title  = correct_path(s->title );
        char * good_path   =  strdup_printf("%s/%s_%s_lyrics_%d.txt",save_dir,good_artist,good_title,i);

        if(good_title)
                free(good_title);
        if(good_artist)
                free(good_artist);

        return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_photos(GlyQuery * s, const char * save_dir, int i)
{
        char * good_artist = correct_path(s->artist);
        char * good_path   = strdup_printf("%s/%s_photo_%d.jpg",save_dir,good_artist,i);

        if(good_artist)
                free(good_artist);

        return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_ainfo(GlyQuery * s, const char * save_dir, int i)
{
        char * good_artist = correct_path(s->artist);
        char * good_path   = strdup_printf("%s/%s_ainfo_%d.txt",save_dir,good_artist,i);

        if(good_artist)
                free(good_artist);

        return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_similiar(GlyQuery *s, const char * save_dir, int i)
{
        char * good_artist = correct_path(s->artist);
        char * good_path   = strdup_printf("%s/%s_similiar_%d.txt",save_dir, good_artist,i);

        if(good_artist)
                free(good_artist);

        return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_album_artist(GlyQuery *s, const char * save_dir, int i, const char * type)
{
        char * good_artist = correct_path(s->artist);
        char * good_album  = correct_path(s->album );
        char * good_path   = strdup_printf("%s/%s_%s_%s_%d.txt",save_dir,good_artist,good_album,type,i);

        if(good_artist)
                free(good_artist);
        if(good_album)
                free(good_album);

        return good_path;
}

/* --------------------------------------------------------- */
// ---------------------- MISC ----------------------------- //
/* --------------------------------------------------------- */

static char * path_review(GlyQuery *s, const char * save_dir, int i)
{
        return path_album_artist(s,save_dir,i,"review");
}

static char * path_tracklist(GlyQuery *s, const char * save_dir, int i)
{
        return path_album_artist(s,save_dir,i,"tracktitle");
}

static char * path_albumlist(GlyQuery *s, const char * save_dir, int i)
{
        char * good_artist = correct_path(s->artist);
        char * good_path   = strdup_printf("%s/%s_albumtitle_%d.txt",save_dir,good_artist,i);
        if(good_artist)
                free(good_artist);

        return good_path;
}

static char * path_tags(GlyQuery *s, const char * save_dir, int i)
{
        char * good_artist = correct_path(s->artist);
        char * good_path   = strdup_printf("%s/%s_tag_%d.txt",save_dir,s->artist,i);

        if(good_artist)
                free(good_artist);

        return good_path;
}

static char * path_relations(GlyQuery *s, const char * save_dir, int i)
{
        char * good_artist = correct_path(s->artist);
        char * good_path   = strdup_printf("%s/%s_url_%d.txt",save_dir,s->artist,i);

        if(good_artist)
                free(good_artist);

        return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

char * get_path_by_type(GlyQuery * s, const char * sd, int iter)
{
        char * m_path = NULL;
        switch(s->type) {
        case GET_COVERART:
                m_path = path_covers(s,sd,iter);
                break;
        case GET_LYRICS:
                m_path = path_lyrics(s,sd,iter);
                break;
        case GET_ARTIST_PHOTOS:
                m_path = path_photos(s,sd,iter);
                break;
        case GET_ARTISTBIO:
                m_path = path_ainfo(s,sd,iter);
                break;
        case GET_SIMILIAR_ARTISTS:
                m_path = path_similiar(s,sd,iter);
                break;
        case GET_ALBUM_REVIEW:
                m_path = path_review(s,sd,iter);
                break;
        case GET_TRACKLIST:
                m_path = path_tracklist(s,sd,iter);
                break;
        case GET_ALBUMLIST:
                m_path = path_albumlist(s,sd,iter);
                break;
        case GET_TAGS:
                m_path = path_tags(s,sd,iter);
                break;
        case GET_RELATIONS:
                m_path = path_relations(s,sd,iter);
                break;
	case GET_UNSURE: 
		glyr_message(-1,NULL,stderr,"getPath(): Unknown type, Problem?\n");
        }
        return m_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* -------------------------------------------------------- */

static void print_item(GlyQuery *s, GlyMemCache * cacheditem, int num)
{
        // GlyMemcache members
        // dsrc = Exact link to the location where the data came from
        // size = size in bytes
        // type = Type of data
        // data = actual data
        // (error) - Don't use this. Only internal use
        glyr_message(1,s,stderr,"\n------- ITEM #%d --------\n",num);
        glyr_message(1,s,stderr,"FROM: <%s>\n",cacheditem->dsrc);
        glyr_message(1,s,stderr,"PROV: %s\n",cacheditem->prov);
        glyr_message(1,s,stderr,"SIZE: %d Bytes\n",(int)cacheditem->size);
	glyr_message(1,s,stderr,"MSUM: ");

        int i;
        for (i = 0; i < 16; i++) {
                glyr_message(1,s,stderr,"%02x", cacheditem->md5sum[i]);
        }
 
        glyr_message(1,s,stderr,"\nTYPE: ");

        // Each cache identfies it's data by a constant
        switch(cacheditem->type) {
        case TYPE_COVER:
                glyr_message(1,s,stderr,"cover");
                break;
        case TYPE_COVER_PRI:
                glyr_message(1,s,stderr,"cover (frontside)");
                break;
        case TYPE_COVER_SEC:
                glyr_message(1,s,stderr,"cover (backside or inlet)");
                break;
        case TYPE_LYRICS:
                glyr_message(1,s,stderr,"songtext");
                break;
        case TYPE_PHOTOS:
                glyr_message(1,s,stderr,"band photo");
                break;
        case TYPE_REVIEW:
                glyr_message(1,s,stderr,"albumreview");
                break;
        case TYPE_AINFO:
                glyr_message(1,s,stderr,"artistbio");
                break;
        case TYPE_SIMILIAR:
                glyr_message(1,s,stderr,"similiar artist");
                break;
        case TYPE_TRACK:
                glyr_message(1,s,stderr,"trackname [%d:%02d]",cacheditem->duration/60,cacheditem->duration%60);
                break;
        case TYPE_ALBUMLIST:
                glyr_message(1,s,stderr,"albumname");
                break;
        case TYPE_TAGS:
                glyr_message(1,s,stderr,"some tag");
                break;
        case TYPE_TAG_ARTIST:
                glyr_message(1,s,stderr,"artisttag");
                break;
        case TYPE_TAG_ALBUM:
                glyr_message(1,s,stderr,"albumtag");
                break;
        case TYPE_TAG_TITLE:
                glyr_message(1,s,stderr,"titletag");
                break;
        case TYPE_RELATION:
                glyr_message(1,s,stderr,"relation");
                break;
        case TYPE_NOIDEA:
        default:
                glyr_message(1,s,stderr,"No idea...?");
        }

        // Print the actual data.
        // This might have funny results if using cover/photos
        if(!cacheditem->is_image)
                glyr_message(1,s,stderr,"\nDATA:\n%s",cacheditem->data);
        else
                glyr_message(1,s,stderr,"\nDATA: <not printable>");

        glyr_message(1,s,stderr,"\n------------------------\n");
        glyr_message(1,s,stderr,"\n");
}

/* --------------------------------------------------------- */

static const char * get_type_string(GlyQuery * s)
{
	const char **   cp_names = GlyPlug_get_name_by_id(GET_UNSURE);
	unsigned char * cp_gids  = GlyPlug_get_gid_by_id(GET_UNSURE);	

	const char * result = NULL;

        if(s && cp_names && cp_gids) {
                int i = 0;
                for(i = 0; cp_names[i] != NULL && !result; i++) {
                        if(s->type == cp_gids[i]) {
                                result = strdup(cp_names[i]);
                        }
                }

		free_name_list(cp_names);
		free(cp_gids);
        }
        return result;
}

/* --------------------------------------------------------- */

// Increaase to get correcter results, but causes more traffic
static void make_translation_work(GlyQuery * s, GlyMemCache * to_translate)
{
	if(s && to_translate && !to_translate->is_image && s->gtrans.target) {
			if(!s->gtrans.source || strcmp(s->gtrans.target,s->gtrans.source)) {
				Gly_gtrans_translate(s,to_translate);
			} else {
				glyr_message(1,s,stderr,"- The target lang is the same as the source lang - Ignore'd.");
			}
	}
}
/* --------------------------------------------------------- */

static enum GLYR_ERROR callback(GlyMemCache * c, GlyQuery * s)
{
        // This is just to demonstrate the callback option.
        // Put anything in here that you want to be executed when
        // a cache is 'ready' (i.e. ready for return)
        // See the glyr_set_dl_callback for more info
        // a custom pointer is in s->callback.user_pointer
        int * i = s->callback.user_pointer;

	// Realtime translation (fun, but mostly useless)
	make_translation_work(s,c);

	// Text Represantation of this item
        print_item(s,c,(*i));

        /* write out 'live' */
        if(write_arg) {
                size_t opt = 0;
                for(opt = 0; write_arg[opt]; opt++) {
                        char * path = get_path_by_type(s,write_arg[opt],*i);
                        if(path != NULL) {
				const char * data_type = get_type_string(s);
				if(data_type != NULL) {
                                	glyr_message(1,s,stderr,"- Writing '%s' to %s\n",data_type,path);
					free((char*)data_type);
				}

                                if(Gly_write(c,path) == -1) {
                                        glyr_message(1,s,stderr,"(!!) glyrc: writing data to <%s> failed.\n",path);
                                }
                        }

                        /* call the program if any specified */
                        if(exec_on_call != NULL) {
                                char * replace_path = strdup(exec_on_call);
                                if(path != NULL) {
                                        replace_path = strreplace(exec_on_call,"<path>",path);
                                }

                                // Call command
                                int exitVal = system(replace_path);

                                if(exitVal != EXIT_SUCCESS) {
                                        glyr_message(1,s,stderr,"glyrc: cmd returned a value != EXIT_SUCCESS\n");
                                }

                                free(replace_path);
                        }
                        free(path);
                }
        }

	if(i != NULL) {
        	*i += 1;
	} else {
		glyr_message(-1,NULL,stderr,"warning: Empty counterpointer!\n");
	}
        return GLYRE_OK;
}

/* --------------------------------------------------------- */

static void print_suppported_languages(GlyQuery * s)
{
	size_t i = 0;
	char ** lang_list = Gly_gtrans_list(s);

	// Print all in a calendarstyle format
	while(lang_list[i]) {
		if(i % 10 == 0) {
			glyr_message(2,s,stderr,"\n");
		}
		glyr_message(2,s,stderr,"%s ",lang_list[i]);

		// free this field
		free(lang_list[i]);
		lang_list[i] = NULL;

		i++;
	}
	if(lang_list != NULL) {
		free(lang_list);
	}

	glyr_message(2,s,stderr,"\n\n");
}

/* --------------------------------------------------------- */

int main(int argc, char * argv[])
{
        int result = EXIT_SUCCESS;

        /* Warn on  crash */
        signal(SIGSEGV, sig_handler);
        signal(SIGINT,  sig_handler);

        // make sure to init everything and destroy again
        Gly_init();
        atexit(Gly_cleanup);

	/* Go firth unless the user demeands translation */
        if(argc >= 3 && strcmp(argv[1],"gtrans") != 0) {
		// The struct that control this beast
                GlyQuery my_query;

                // set it on default values
                Gly_init_query(&my_query);

		// Good enough for glyrc
                GlyOpt_verbosity(&my_query,2);

                // Set the type..
                if(!set_get_type(&my_query, argv[1])) {
                        Gly_destroy_query( &my_query );
                        return EXIT_FAILURE;
                }

                write_arg = parse_commandline_general(argc-1, argv+1, &my_query);
                if(write_arg == NULL) {
                        write_arg = malloc(2 * sizeof(char*));
                        write_arg[0] = strdup(default_path);
                        write_arg[1] = NULL;
                }

		// Special cases
                if(my_query.type == GET_ARTISTBIO)
			GlyOpt_number(&my_query,my_query.number*2);

                // Check if files do already exist
                bool file_exist = false;

                int iter = 0;
                for(iter = 0; iter < my_query.number; iter++) {
                        size_t j = 0;
                        for(j = 0; write_arg[j]; j++) {
                                char * path = get_path_by_type(&my_query, write_arg[j], iter);
                                if(path) {
                                        if(!update && (file_exist = !access(path,R_OK) )) {
                                                free(path);
                                                break;
                                        }
                                        free(path);
                                        path = NULL;
                                }
                        }
                }

		if(my_query.type == GET_TRACKLIST)
			GlyOpt_number(&my_query,0);

		if(my_query.type == GET_ALBUMLIST)
			GlyOpt_number(&my_query,0);

                // Set the callback - it will do all the actual work
                int item_counter = 0;
                GlyOpt_dlcallback(&my_query, callback, &item_counter);

                if(my_query.type != GET_UNSURE) {
                        if(!file_exist) {

                                // Now start searching!
				int length = -1;
                                enum GLYR_ERROR get_error = GLYRE_OK;
                                GlyMemCache * my_list= Gly_get(&my_query, &get_error, &length);

                                if(my_list) {
                                        if(get_error == GLYRE_OK) {
                                                /* This is the place where you would work with the cachelist *
                                                   As the callback is used in glyrc this is just plain empty *
                                                   Useful if you need to cache the data (e.g. for batch jobs *
						   Left only for the reader's informatiom, no functions here *
                                                */
						glyr_message(2,&my_query,stderr,"In total %d items found.\n",length);
                                        }

                                        // Free all downloaded buffers
                                        Gly_free_list(my_list);
                                } else if(get_error != GLYRE_OK) {
                                        glyr_message(1,&my_query,stderr,"E: %s\n",Gly_strerror(get_error));
                                }
                        } else {
                                glyr_message(1,&my_query,stderr,C_B"*"C_" File(s) already exist. Use -u to update.\n");
                        }

			// free pathes
                        size_t x = 0;
                        for( x = 0; write_arg[x]; x++) {
                                free((char*)write_arg[x]);
                                write_arg[x] = NULL;
                        }
                        free(write_arg);
                        write_arg = NULL;

                        // Clean memory alloc'd by settings
                        Gly_destroy_query( &my_query);
                }
	/* Translator mode - simple interface to google translator */
	} else if(argc >= 3 && !strcmp(argv[1],"gtrans")) {
		GlyQuery settings;
		Gly_init_query(&settings);
		GlyOpt_verbosity(&settings,2);

		/* List supported languages */
		if(!strcmp(argv[2],"list")) {
			print_suppported_languages(&settings);

		/* detect language snippet given as argument */
		} else if(!strcmp(argv[2],"detect") && argc >= 4) {
			float correctness = 0.0;
			char * lang_guess = Gly_gtrans_lookup(&settings,argv[3],&correctness);
			if(lang_guess != NULL) {
				glyr_message(2,&settings,stdout,"%s (%3.2f%% probability)\n",lang_guess,correctness*100.0);
				free(lang_guess);
			}

		/* Translation */
		} else if(argc >= 4) {
			GlyMemCache * buffer = Gly_new_cache();
			if(buffer != NULL) {
				buffer->data = strdup(argv[2]);
				buffer->size = strlen(argv[2]);

				if(strcmp(argv[3],"auto") != 0) {
					GlyOpt_gtrans_source_lang(&settings,argv[3]);
				} /* else autodetect */

				/* set target language */
				GlyOpt_gtrans_target_lang(&settings,argv[4]);

				Gly_gtrans_translate(&settings,buffer);
				glyr_message(2,&settings,stdout,"%s\n",buffer->data);

				/* all done - bye! */
				Gly_free_cache(buffer);
			}
		} else { /* usage */
			glyr_message(-1,NULL,stderr,"Usage:\n");
			glyr_message(-1,NULL,stderr,"  glyrc gtrans list\n");
			glyr_message(-1,NULL,stderr,"  glyrc gtrans [text] [sourcelang|auto] [targetlang]\n");
			glyr_message(-1,NULL,stderr,"  glyrc gtrans detect [text]\n\n");
		}

		/* free all registers */
		Gly_destroy_query(&settings);

	/*  making glyrc -h works (*sigh*) */
        } else if(argc >= 2 && !strcmp(argv[1],"-V")) {
                print_version(NULL);
        } else if(argc >= 2 && (!strcmp(argv[1],"-C") || !strcmp(argv[1],"-CH"))) {
                GlyQuery tmp;
                tmp.color_output = false;
                usage(&tmp);
        } else if(argc >= 2 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"-ch") || !strcmp(argv[1],"-hc"))) {
                help_short(NULL);
        } else {
                GlyQuery tmp;
                tmp.color_output = false;
                usage(&tmp);
        }

        // byebye
        return result;
}

//* --------------------------------------------------------- */
// ------------------End of program-------------------------- //
/* ---------------------------------------------------------- */