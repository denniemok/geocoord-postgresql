/******************************************************************************
  Completed with reference to https://www.postgresql.org/docs/current/extend.html
******************************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"
#include "access/hash.h"
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <regex.h>
#include <math.h>

PG_MODULE_MAGIC;

typedef struct {
    int32 length;
	int32 latDir;
	int32 longDir;
	float8 latitude;
	float8 longitude;
	char locName[FLEXIBLE_ARRAY_MEMBER];
} GeoCoord;


/*****************************************************************************
  Input/Output Functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(gcoord_in);

Datum gcoord_in(PG_FUNCTION_ARGS) {
	
	char* str = PG_GETARG_CSTRING(0);
    GeoCoord* result;
	regex_t regex;
    int invalid = 0;
	int n_part = 0;
    char* part[5];
    char* token;
    double x = 0;
    double y = 0;
    char* end;
    int x_mode = 0;
    int y_mode = 0;

	// get a copy of raw string
	char* str2 = (char*) palloc(strlen(str) + 1);
	strcpy(str2, str); // to avoid original string being modified

	// REGEX CHECK
	// https://stackoverflow.com/questions/1085083/regular-expressions-in-c-examples

	// compile regex
    // regex made with the help from https://regexr.com/
    if (regcomp(&regex, "^[A-Za-z]+( [A-Za-z]+)*,[0-9]*(\\.[0-9]{1,4})?°[N,S,E,W][, ][0-9]*(\\.[0-9]{1,4})?°[N,S,E,W]$", REG_EXTENDED) == 0) {
        // execute regex
        if (regexec(&regex, str2, 0, NULL, 0) == 0) {
        	//elog(INFO, "Regex matched."); 
    	} else {
    		//elog(INFO, "Regex mismatched.");
    		invalid = 1;
    	}
    } else {
    	//elog(INFO, "Failed to compile regex."); 
        invalid = 1;
    }

    // free regex
    regfree(&regex);

    if (invalid == 1) {
    	ereport(ERROR,(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
    		errmsg("invalid input syntax for type %s: \"%s\"", "GeoCoord", str)));
    }

    // split string into different parts
    // https://www.geeksforgeeks.org/how-to-split-a-string-in-cc-python-and-java/
    
    token = strtok(str2, ",° ");
    while (token != NULL) {
        //elog(INFO, "%lu|%s", strlen(token), token); // print length of part and part string

        if (n_part < 5) { // index 0 - 4
        	if (n_part == 1 && isdigit(token[0]) == 0) { // if locName has multiple parts
        		char* a = (char*) palloc(strlen(part[0]) + strlen(token) + 1 + 1);
        		strcpy(a, strcat(strcat(part[0], " "), token));
        		part[0] = a;
        		//elog(INFO, "%li|%s", strlen(part[0]), part[0]);
        		n_part--;
        	} else {
        		part[n_part] = (char*) palloc(strlen(token) + 1);
	        	strcpy(part[n_part], token);
	        }
    	}

        token = strtok(NULL, ",° ");
        n_part++;
    }

    //elog(INFO, "n_part: %i", n_part); // print number of n_part

    // change locName to lower case
    for (int i = 0; i < strlen(part[0]); i++) { 
    	part[0][i] = tolower(part[0][i]);
    }

    //elog(INFO, "tolower: %s", part[0]);

    // check if direction is correct
    // and store in canonical form
    if (strcmp(part[2], "S") == 0 || strcmp(part[2], "N") == 0) {
    	if (strcmp(part[4], "E") == 0 || strcmp(part[4], "W") == 0) {
    		x = strtod(part[1], &end);
    		y = strtod(part[3], &end);
    		if (x > 90 || y > 180) {
    			invalid = 1;
    		} else {
    			if (strcmp(part[2], "S") == 0) {
    				x_mode = 0;
    			} else {
    				x_mode = 1; // N, because N > S
    			}
    			if (strcmp(part[4], "W") == 0) {
    				y_mode = 0;
    			} else {
    				y_mode = 1; // E, because E > W
    			}
    		}
    	}
    } else if (strcmp(part[2], "E") == 0 || strcmp(part[2], "W") == 0) {
    	if (strcmp(part[4], "S") == 0 || strcmp(part[4], "N") == 0) {
    		y = strtod(part[1], &end);
    		x = strtod(part[3], &end);
    		if (x > 90 || y > 180) {
    			invalid = 1;
    		} else {
    			if (strcmp(part[2], "W") == 0) {
    				y_mode = 0;
    			} else {
    				y_mode = 1; // E, because E > W
    			}
    			if (strcmp(part[4], "S") == 0) {
    				x_mode = 0;
    			} else {
    				x_mode = 1; // N, because N > S
    			}
    		}
    	}
    } else {
    	invalid = 1;
    }

    // if invalid
    if (invalid == 1) {
    	ereport(ERROR,(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
    		errmsg("invalid input syntax for type %s: \"%s\"", "GeoCoord", str)));
    }

    // struct has padding!
	result = (GeoCoord *) palloc(VARHDRSZ+2*sizeof(int)+2*sizeof(double)+(strlen(part[0])+1)*sizeof(char)+4);
	SET_VARSIZE(result, VARHDRSZ+2*sizeof(int)+2*sizeof(double)+(strlen(part[0])+1)*sizeof(char)+4);
	result->latDir = x_mode;
    result->longDir = y_mode;
    result->latitude = x;
	result->longitude = y;
	strcpy(result->locName, part[0]);
    // elog(INFO, "%s", result->locName);
    // elog(INFO, "%lf", result->latitude);
    // elog(INFO, "%lf", result->longitude);
    // elog(INFO, "%i", result->latDir);
    // elog(INFO, "%i", result->longDir);
	PG_RETURN_POINTER(result);

}


PG_FUNCTION_INFO_V1(gcoord_out);

Datum gcoord_out(PG_FUNCTION_ARGS) {
	
    GeoCoord* gcoord = (GeoCoord*) PG_GETARG_POINTER(0);
	char* result;
    char latDir[2];
    char longDir[2];

	if (gcoord->latDir == 0) {
		strcpy(latDir, "S");
	} else {
		strcpy(latDir, "N"); // N > S
	}

	if (gcoord->longDir == 0) {
		strcpy(longDir, "W");
	} else {
		strcpy(longDir, "E"); // E > W
	}

	result = psprintf("%s,%lf°%s,%lf°%s", gcoord->locName, gcoord->latitude, latDir, gcoord->longitude, longDir);
	PG_RETURN_CSTRING(result);
    
}

/*****************************************************************************
  Other Functions
 *****************************************************************************/

// C MIN
// https://stackoverflow.com/questions/3437404/min-and-max-in-c
#define min(x,y) (((x)<(y))?(x):(y))

static int lexically_cmp(GeoCoord* a, GeoCoord* b) {
    
    int len = min(strlen(a->locName),strlen(a->locName));

    //elog(INFO, "Comparing %s, %s", a->locName, b->locName);

    // compare character by character with the smallest string
    for (int i = 0; i < len; i++) {
        if (a->locName[i] > b->locName[i]) {
            //elog(INFO, "%c, %c, 1", a->locName[i], b->locName[i]);
            return 1; // if lexically greater in terms of alphabet
        } else if (a->locName[i] < b->locName[i]) {
            //elog(INFO, "%c, %c, -1", a->locName[i], b->locName[i]);
            return -1; // if lexically smaller in terms of alphabet
        }
    }

    // if the first n characters are the same, compare their length
    if (strlen(a->locName) > strlen(b->locName)) {
        //elog(INFO, "%lu, %lu, 1", strlen(a->locName), strlen(b->locName));
        return 1; // if lexically greater in terms of length
    } else if (strlen(a->locName) < strlen(b->locName)) {
        //elog(INFO, "%lu, %lu, -1", strlen(a->locName), strlen(b->locName));
        return -1; // if lexically smaller in terms of length
    }

    // lexically equal to
    //elog(INFO, "%s = %s", a->locName, b->locName);
    return 0;

}

static int gcoord_cmp(GeoCoord* a, GeoCoord* b) {

    // start by comparing latitude
    // first compare magnitude
    if (-a->latitude > -b->latitude) { // if near equator
        //elog(INFO, "%lf, %lf, 1", a->latitude, b->latitude);
        return 1;
    } else if (-a->latitude < -b->latitude) { // if further away from equator
        //elog(INFO, "%lf, %lf, -1", a->latitude, b->latitude);
        return -1;
    } else {
        //elog(INFO, "%lf, %lf, 0", a->latitude, b->latitude);
    }

    // if magniturde are the same, compare direction
    if (a->latDir > b->latDir) { // if north
        //elog(INFO, "%i, %i, 1", a->latDir, b->latDir);
        return 1;
    } else if (a->latDir < b->latDir) { // if south
        //elog(INFO, "%i, %i, -1", a->latDir, b->latDir);
        return -1;
    } else {
        //elog(INFO, "%i, %i, 0", a->latDir, b->latDir);
    }

    // if latitudes are the same, compare longitude
    // first compare magnitude
    if (-a->longitude > -b->longitude) { // if near prime meridian
        //elog(INFO, "%lf, %lf, 1", a->longitude, b->longitude);
        return 1;
    } else if (-a->longitude < -b->longitude) { // if further away from prime meridian
        //elog(INFO, "%lf, %lf, -1", a->longitude, b->longitude);
        return -1;
    } else {
        //elog(INFO, "%lf, %lf, 0", a->longitude, b->longitude);
    }

    // if magniturde are the same, compare direction
    if (a->longDir > b->longDir) { // if east
        //elog(INFO, "%i, %i, 1", a->longDir, b->longDir);
        return 1;
    } else if (a->longDir < b->longDir) { // if west
        //elog(INFO, "%i, %i, -1", a->longDir, b->longDir);
        return -1;
    } else {
        //elog(INFO, "%i, %i, 0", a->longDir, b->longDir);
    }

    // if longitudes are the same, compare locName lexically
    return lexically_cmp(a,b);

}

static int timezone_cmp(GeoCoord* a, GeoCoord* b) {

    int timezone_a;
    int timezone_b;

    if (a->longDir == 1) { // if E
        timezone_a = (int)floor(a->longitude / 15);
    } else { // if W
        timezone_a = -(int)floor(a->longitude / 15);
    }

    if (b->longDir == 1) { // if E
        timezone_b = (int)floor(b->longitude / 15);
    } else { // if W
        timezone_b = -(int)floor(b->longitude / 15);
    }

    //elog(INFO, "%lf, %i, %i", a->longitude, a->longDir, timezone_a);
    //elog(INFO, "%lf, %i, %i", b->longitude, b->longDir, timezone_b);

    if (timezone_a == timezone_b) {
        return 0;
    } else {
        return -1;
    }

}


// support function
PG_FUNCTION_INFO_V1(gcoord_s_cmp);

Datum gcoord_s_cmp(PG_FUNCTION_ARGS) {
    
    GeoCoord* a = (GeoCoord *) PG_GETARG_POINTER(0);
    GeoCoord* b = (GeoCoord *) PG_GETARG_POINTER(1);

    PG_RETURN_INT32(gcoord_cmp(a,b));

}


PG_FUNCTION_INFO_V1(gcoord_eq); // equal to

Datum gcoord_eq(PG_FUNCTION_ARGS) {

    GeoCoord* a = (GeoCoord *) PG_GETARG_POINTER(0);
    GeoCoord* b = (GeoCoord *) PG_GETARG_POINTER(1);

    //elog(INFO, "EQUAL TO");
    PG_RETURN_BOOL(gcoord_cmp(a,b) == 0);

}


PG_FUNCTION_INFO_V1(gcoord_neq); // not equal to

Datum gcoord_neq(PG_FUNCTION_ARGS) {

    GeoCoord* a = (GeoCoord *) PG_GETARG_POINTER(0);
    GeoCoord* b = (GeoCoord *) PG_GETARG_POINTER(1);

    //elog(INFO, "NOT EQUAL TO");
    PG_RETURN_BOOL(gcoord_cmp(a,b) != 0);

}


PG_FUNCTION_INFO_V1(gcoord_lt); // less than

Datum gcoord_lt(PG_FUNCTION_ARGS) {

	GeoCoord* a = (GeoCoord *) PG_GETARG_POINTER(0);
    GeoCoord* b = (GeoCoord *) PG_GETARG_POINTER(1);

    //elog(INFO, "LESS THAN");
    PG_RETURN_BOOL(gcoord_cmp(a,b) < 0);

}


PG_FUNCTION_INFO_V1(gcoord_le); // less than or equal to

Datum gcoord_le(PG_FUNCTION_ARGS) {

	GeoCoord* a = (GeoCoord *) PG_GETARG_POINTER(0);
    GeoCoord* b = (GeoCoord *) PG_GETARG_POINTER(1);

    //elog(INFO, "LESS THAN OR EQUAL TO");
    PG_RETURN_BOOL(gcoord_cmp(a,b) <= 0);

}


PG_FUNCTION_INFO_V1(gcoord_gt); // greater than

Datum gcoord_gt(PG_FUNCTION_ARGS) {

    GeoCoord* a = (GeoCoord *) PG_GETARG_POINTER(0);
    GeoCoord* b = (GeoCoord *) PG_GETARG_POINTER(1);

    //elog(INFO, "GREATER THAN");
    PG_RETURN_BOOL(gcoord_cmp(a,b) > 0);

}


PG_FUNCTION_INFO_V1(gcoord_ge); // greater than or equal to

Datum gcoord_ge(PG_FUNCTION_ARGS) {
    
	GeoCoord* a = (GeoCoord *) PG_GETARG_POINTER(0);
    GeoCoord* b = (GeoCoord *) PG_GETARG_POINTER(1);

    //elog(INFO, "GREATER THAN OR EQUAL TO");
    PG_RETURN_BOOL(gcoord_cmp(a,b) >= 0);

}


PG_FUNCTION_INFO_V1(gcoord_eqtz); // equal timezone

Datum gcoord_eqtz(PG_FUNCTION_ARGS) {
    
    GeoCoord* a = (GeoCoord *) PG_GETARG_POINTER(0);
    GeoCoord* b = (GeoCoord *) PG_GETARG_POINTER(1);

    //elog(INFO, "EQUAL TIME ZONE");
    PG_RETURN_BOOL(timezone_cmp(a,b) == 0);

}


PG_FUNCTION_INFO_V1(gcoord_neqtz); // not equal timezone

Datum gcoord_neqtz(PG_FUNCTION_ARGS) {
    
    GeoCoord* a = (GeoCoord *) PG_GETARG_POINTER(0);
    GeoCoord* b = (GeoCoord *) PG_GETARG_POINTER(1);

    //elog(INFO, "NOT EQUAL TIME ZONE");
    PG_RETURN_BOOL(timezone_cmp(a,b) != 0);

}


PG_FUNCTION_INFO_V1(convert2dms);

Datum convert2dms(PG_FUNCTION_ARGS) {
    
    GeoCoord* gcoord = (GeoCoord *) PG_GETARG_POINTER(0);
    char* result;
    char latDir[2];
    char longDir[2];

    int D_a, D_b;
    int M_a, M_b;
    int S_a, S_b;

    D_a = (int)floor(gcoord->latitude);
    D_b = (int)floor(gcoord->longitude);

    // add small offset to fix rounding issue
    // https://stackoverflow.com/questions/7657326/how-to-convert-double-to-int-in-c#:~:text=double%20a%3B%20a%20%3D%203669.0%3B,in%20b%2C%20instead%20of%203669.
    M_a = (int)floor(60 * fabs(gcoord->latitude - D_a) + 1e-9);
    M_b = (int)floor(60 * fabs(gcoord->longitude - D_b) + 1e-9);

    S_a = (int)floor(3600 * fabs(gcoord->latitude - D_a) - 60 * M_a + 1e-9);
    S_b = (int)floor(3600 * fabs(gcoord->longitude - D_b) - 60 * M_b + 1e-9);

    if (gcoord->latDir == 0) {
        strcpy(latDir, "S");
    } else {
        strcpy(latDir, "N"); // N > S
    }

    if (gcoord->longDir == 0) {
        strcpy(longDir, "W");
    } else {
        strcpy(longDir, "E"); // E > W
    }

    // 9 possibilities
    if (M_a != 0 && S_a != 0 && M_b != 0 && S_b != 0) { // TTTT
        result = psprintf("%s,%i°%i'%i\"%s,%i°%i'%i\"%s", gcoord->locName, D_a, M_a, S_a, latDir, D_b, M_b, S_b, longDir);
    } else if (M_a != 0 && S_a != 0 && M_b != 0 && S_b == 0) { // TTTF
        result = psprintf("%s,%i°%i'%i\"%s,%i°%i'%s", gcoord->locName, D_a, M_a, S_a, latDir, D_b, M_b, longDir);
    } else if (M_a != 0 && S_a != 0 && M_b == 0 && S_b == 0) { // TTFF
        result = psprintf("%s,%i°%i'%i\"%s,%i°%s", gcoord->locName, D_a, M_a, S_a, latDir, D_b, longDir);
    
    } else if (M_a != 0 && S_a == 0 && M_b != 0 && S_b != 0) { // TFTT
        result = psprintf("%s,%i°%i'%s,%i°%i'%i\"%s", gcoord->locName, D_a, M_a, latDir, D_b, M_b, S_b, longDir);
    } else if (M_a != 0 && S_a == 0 && M_b != 0 && S_b == 0) { // TFTF
        result = psprintf("%s,%i°%i'%s,%i°%i'%s", gcoord->locName, D_a, M_a, latDir, D_b, M_b, longDir);
    } else if (M_a != 0 && S_a == 0 && M_b == 0 && S_b == 0) { // TFFF
        result = psprintf("%s,%i°%i'%s,%i°%s", gcoord->locName, D_a, M_a, latDir, D_b, longDir);

    } else if (M_a == 0 && S_a == 0 && M_b == 0 && S_b == 0) { // FFFF
        result = psprintf("%s,%i°%s,%i°%s", gcoord->locName, D_a, latDir, D_b, longDir);
    } else if (M_a == 0 && S_a == 0 && M_b != 0 && S_b != 0) { // FFTT
        result = psprintf("%s,%i°%s,%i°%i'%i\"%s", gcoord->locName, D_a, latDir, D_b, M_b, S_b, longDir);
    } else if (M_a == 0 && S_a == 0 && M_b != 0 && S_b == 0) { // FFTF
        result = psprintf("%s,%i°%s,%i°%i'%s", gcoord->locName, D_a, latDir, D_b, M_b, longDir);
    } else {
        result = psprintf("%s,%i°%i'%i\"%s,%i°%i'%i\"%s", gcoord->locName, D_a, M_a, S_a, latDir, D_b, M_b, S_b, longDir);
    }
    
    PG_RETURN_CSTRING(result);

}


PG_FUNCTION_INFO_V1(gcoord_hash);

Datum gcoord_hash(PG_FUNCTION_ARGS) {

    GeoCoord* gcoord = (GeoCoord *) PG_GETARG_POINTER(0);

    PG_RETURN_INT32(DatumGetUInt32(hash_any((unsigned char*) gcoord->locName, strlen(gcoord->locName))));

}