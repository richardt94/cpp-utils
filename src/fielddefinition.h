/*
This source code file is licensed under the GNU GPL Version 2.0 Licence by the following copyright holder:
Crown Copyright Commonwealth of Australia (Geoscience Australia) 2015.
The GNU GPL 2.0 licence is available at: http://www.gnu.org/licenses/gpl-2.0.html. If you require a paper copy of the GNU GPL 2.0 Licence, please write to Free Software Foundation, Inc. 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

Author: Ross C. Brodie, Geoscience Australia.
*/

#ifndef cfielddefinitionH
#define cfielddefinitionH

#include <stdlib.h>
#include <cstring>
#include <vector>
#include <float.h>

#include "general_utils.h"
#include "blocklanguage.h"
#include "asciicolumnfile.h"

enum eFieldDefinitionType {VARIABLENAME,COLUMNNUMBER,NUMERIC,UNAVAILABLE};

class cFieldDefinition{

private:
	
	bool isnumeric(const std::string& rhs)
	{
		std::vector<std::string> tokens = tokenise(rhs, ' ');
		if (tokens.size() == 0)return false;

		char* str_end;
		std::strtod(tokens[0].c_str(), &str_end);
		if (*str_end) return false;
		return true;
	};

public:

	size_t coff = 1;//First column in ascii files for user perspective
	eFieldDefinitionType deftype;
	char op = ' ';
	double opval = 0.0;
	bool flip = false;

	std::string varname;//Netcdf variable name
	size_t column;//Ascii file start column number
	std::vector<double> numericvalue;//Numeric value

	
	cFieldDefinition() { }

	eFieldDefinitionType definitiontype() const {
		return deftype;
	}
	
	void initialise(const cBlock& b, const std::string& key)
	{
		int col;
		std::string rhs = b.getstringvalue(key);
		if (rhs == ud_string()) {
			deftype = UNAVAILABLE;			
		}
		else if(isnumeric(rhs)){
			deftype = NUMERIC;
			column = (size_t)0;
			numericvalue = b.getdoublevector(key);			
		}
		else if (strncasecmp(rhs, "Unavailable", 11) == 0) {
			deftype = UNAVAILABLE;
			column = 0;			
		}
		else if (strncasecmp(rhs, "Column", 6) == 0){			
			deftype = COLUMNNUMBER;		
			flip = false;
			int n = sscanf(&(rhs.c_str()[6]), "%d %c %lf", &col, &op, &opval);
			column = (size_t)col;
			if (n == 1){ op = ' '; opval = 0.0; }
		}
		else if (strncasecmp(rhs, "-Column", 7) == 0){
			deftype = COLUMNNUMBER;
			flip = true;
			int n = sscanf(&(rhs.c_str()[7]), "%d %c %lf", &col, &op, &opval);
			column = (size_t)col;			
			if (n == 1){ op = ' '; opval = 0.0; }			
		}		
		else{
			if(rhs[0]=='-'){
				deftype = VARIABLENAME;
				flip = true;
				varname = rhs.substr(1, rhs.size()-1);
			}
			else {
				deftype = VARIABLENAME;
				flip = false;
				varname = rhs;
			}			
		}		
	}
		
	template<typename T>
	bool getvalue(const std::vector<std::string>& fields, T& v) const
	{				
		if (deftype == NUMERIC) {
			if (numericvalue.size() == 0) {
				v = undefinedvalue(v);
				return false;
			}
			v = (T)numericvalue[0];
		}
		else if (deftype == COLUMNNUMBER) {						
			std::istringstream(fields[column - coff]) >> v;
			ifnullconvert2zero(v);
			if (flip) v = -1*v;
			applyoperator(v);	
			return true;
		}		
		else if (deftype == VARIABLENAME) {			
			std::istringstream(fields[column - coff]) >> v;
			ifnullconvert2zero(v);
			if (flip) v = -1 * v;
			applyoperator(v);
			return true;
		}
		else if (deftype == UNAVAILABLE) {
			v = undefinedvalue(v);
			return false;
		}
		else {
			//warningglog.logmsg("FieldDefinition::getvalue() unknown column definition\n");
			v = undefinedvalue(v);
			return false;
		}
		return true;
	}

	template<typename T>
	bool getvalue(const std::vector<std::string>& fields, std::vector<T>& vec, const size_t& n) const
	{		
		vec.resize(n);		
		if(deftype == NUMERIC) {
			size_t deflen = numericvalue.size();
			for (size_t i = 0; i < n; i++) {
				if (deflen == 1) vec[i] = (T)numericvalue[0];
				else vec[i] = (T)numericvalue[i];
			}
			return true;
		}
		else if (deftype == COLUMNNUMBER) {	
			for (size_t i = 0; i < n; i++) {				
				std::istringstream(fields[i + column - coff]) >> vec[i];
				if (flip) vec[i] = -vec[i];				
			}	
			return true;
		}		
		else if (deftype == UNAVAILABLE) {
			vec = std::vector<T>(n,undefinedvalue((T)0));
			return false;
		}
		else {			
			vec = std::vector<T>(n, undefinedvalue((T)0));
			return false;
		}		
		return true;
	}
	
	template<typename T>
	bool getvalue(const cAsciiColumnFile& A, std::vector<T>& vec, const size_t& n) const
	{
		vec.resize(n);
		if (deftype == NUMERIC) {
			size_t deflen = numericvalue.size();
			for (size_t i = 0; i < n; i++) {
				if (deflen == 1) vec[i] = (T) numericvalue[0];
				else vec[i] = (T) numericvalue[i];
			}
			return true;
		}
		else if (deftype == COLUMNNUMBER) {
			for (size_t i = 0; i < n; i++) {
				A.getcolumn(i + column - coff, vec[i]);				
				if (flip) vec[i] = -1 * vec[i];
			}
			return true;
		}
		else if (deftype == VARIABLENAME) {
			size_t findex;
			bool status = A.fieldindexbyname(varname,findex);			
			if (status == false) {
				glog.errormsg(_SRC_, "Could not find a field named %s\n", varname.c_str());
			}
			
			bool okstatus = A.getfield(findex, vec);
			if (okstatus == false){
				glog.warningmsg("","Null value %s in field named %s\n", A.fields[findex].nullvaluestr.c_str(), varname.c_str());
				return false;
			}

			for (size_t i = 0; i < n; i++) {				
				if (flip) vec[i] = -1 * vec[i];
			}
			return true;
		}
		else if (deftype == UNAVAILABLE) {
			vec = std::vector<T>(n, undefinedvalue((T)0));
			return false;
		}
		else {
			vec = std::vector<T>(n, undefinedvalue((T)0));
			return false;
		}
		return true;
	}


	template<typename T>
	void ifnullconvert2zero(T& val) const {
		//temporary hack to handle Nulls
		if (val == (T)-999 || val == (T)-9999) val = (T)0;
	}
	
	template<typename T>
	void applyoperator(T& val) const {
		if (op == ' ') return;
		else if (op == '+') val += (T)opval;
		else if (op == '-') val -= (T)opval;
		else if (op == '*') val *= (T)opval;
		else if (op == '/') val /= (T)opval;
		else glog.warningmsg(_SRC_,"Unknown operator %c\n", op);
		return;
	}	
};

#endif
