#define _GNU_SOURCE
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <libxml/xmlreader.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <unistd.h>
#include <ctype.h>	// for isdigit()
#include <stdlib.h>	// for atof()

#include <glib.h>

#include "material.h"
#include "xpu.h"

material_t* materialTable = NULL;
indexSize_t materialTableSize = 0;


void MATRL_Init(indexSize_t tableSize)
{
	materialTableSize = tableSize;
	materialTable = g_malloc(sizeof(material_t) * tableSize);
}


void MATRL_Dump(material_t* mat)
{
	fprintf(stdout,"name: \t%s\n",mat->name);
	fprintf(stdout,"er: \t%f\n",mat->er);
	fprintf(stdout,"cond: %g\n",mat->conductivity);
	fprintf(stdout,"default thickness: %s\n",mat->defaultThickness);
}


void MATRL_DumpAll(void)
{
	indexSize_t i;
	fprintf(stdout,"%s table size: %d\n",__FUNCTION__, materialTableSize);
	for(i=0;i< materialTableSize; i++)
	{
		fprintf(stdout, "index: \t%d\n", i);
		MATRL_Dump(&materialTable[i]);
	}
	fprintf(stdout, "\n");
}

int MATRL_CreateTableFromNodeSet(xmlNodeSetPtr xnsMaterials)
{
	indexSize_t i;
	indexSize_t j = 1;
	if(materialTable == NULL)
		MATRL_Init(xnsMaterials->nodeNr +1);

	materialTable[0].name = (xmlChar*)"default";
	materialTable[0].defaultThickness = (xmlChar*)"0.01";  // in  meters
	materialTable[0].er = 1.0;
	materialTable[0].conductivity = 8.0e-15; // mho/m
	
	for(i=1; i<xnsMaterials->nodeNr +1; i++)
	{
		materialTable[i].name = (xmlChar*)"unused";
		materialTable[i].defaultThickness = (xmlChar*)"0.01";  // in  meters
		materialTable[i].er = 1.0;
		materialTable[i].conductivity = 8.0e-15; // mho/m
	}
	for(i = 0; i<xnsMaterials->nodeNr; i++)
	{
		material_t matCur;
		xmlNodePtr currMaterial = xnsMaterials->nodeTab[i];
		if(currMaterial == NULL)
		{
			return(-1);
		}

		matCur.name = XPU_LookupFromNode(currMaterial, "./@id");
		
		xmlChar* matEr = XPU_LookupFromNode(currMaterial, "./relativePermittivity/text()");
		if(matEr == NULL)
		{
			fprintf(stderr, "\nError, no material Er specified\n");
			return(-1);
		}
		double er = strtod((char*)matEr, NULL);
		xmlFree(matEr);
		matCur.er = er;

		xmlChar* matCond = XPU_LookupFromNode(currMaterial, "./conductivity/text()");
		if(matCond == NULL)
		{
			fprintf(stderr, "\nError, no material conductivity specified\n");
			return(-1);
		}
		double cond = strtod((char*)matCond, NULL);
		xmlFree(matCond);
		matCur.conductivity = cond;
		  			
		xmlChar* cThickness = XPU_LookupFromNode(currMaterial, "./thickness/text()");
		if(cThickness == NULL)
		{
			cThickness = (xmlChar*)"0.001";
		}
		matCur.defaultThickness = cThickness;
		
		if(strcmp((char*)(matCur.name), "air") == 0)
		{ // special case, air replaces the default fill material (vacuum)
			materialTable[0].name = matCur.name;
			materialTable[0].defaultThickness = matCur.defaultThickness;
			materialTable[0].er = matCur.er;
			materialTable[0].conductivity = matCur.conductivity;
		}
		else
		{
			materialTable[j].name = matCur.name;
			materialTable[j].defaultThickness = matCur.defaultThickness;
			materialTable[j].er = matCur.er;
			materialTable[j].conductivity = matCur.conductivity;
			j++;
		}
	}
	return(0);
}

int MATRL_GetIndex(char* name)
{
	indexSize_t retval = 0;
	for(retval = 0; retval<materialTableSize; retval++)
	{
//		fprintf(stdout, "%d, tbl:<%s> for<%s>\n",retval, (char*)materialTable[retval].name, name);
		if(strcmp((char*)materialTable[retval].name, name) == 0)
			return(retval);
	}
	fprintf(stderr, "\nError, index not found for <%s>\n", name);
	return((indexSize_t)(-1));
}

// returns the value converted to meters
double MATRL_ScaleToMeters(double val, char* units) 
{
	if(units == NULL)
		return(val);
	if(strncmp(units, "mm", 2) == 0)
		return(val/1000);
	if(strncmp(units, "mil", 3) == 0)
		return(val*2.54e-5);
	if(strncmp(units, "in", 2) == 0)
		return(val*0.0254);
	if(strncmp(units, "m", 1) == 0)
		return(val);
	if(strncmp(units, "ft", 2) == 0)
		return(val*0.3048);
	fprintf(stderr, "Unknown unit prefix of <%s>\n",units);
		return(val);
}

#define isFloatDigit(x) (isdigit(x) || (x == 'e') || (x == 'E') || (x=='.') || (x=='+') || (x==' ') || (x=='-'))

int MATRL_StringToCounts(char* pString, double metersPerPixel)
{
	char* pend;
	pend = pString;
	if(pend == NULL)
		return(-1);
//	fprintf(stdout, "%s <%s>\n",__FUNCTION__, pString);
	while(	(*pend != '\0') && isFloatDigit(*pend))
	{
		pend++;
	}
	double tempf = atof(pString);
	if(*pend == 0)
		return((int)(tempf/metersPerPixel));  // no units means thickness is in meters

	double retvalf = MATRL_ScaleToMeters( tempf, pend);
	int retval = (int)(retvalf/metersPerPixel);
	if(retval == 0) retval = 1;
//	fprintf(stdout, "%f, %f %d, <%s>\n", tempf, retvalf, retval, pend);
	return(retval);	
	
//	return(-1);
}
		