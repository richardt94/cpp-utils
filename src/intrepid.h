/*
This source code file is licensed under the GNU GPL Version 2.0 Licence by the following copyright holder:
Crown Copyright Commonwealth of Australia (Geoscience Australia) 2015.
The GNU GPL 2.0 licence is available at: http://www.gnu.org/licenses/gpl-2.0.html. If you require a paper copy of the GNU GPL 2.0 Licence, please write to Free Software Foundation, Inc. 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

Author: Ross C. Brodie, Geoscience Australia.
*/

#ifndef _intrepid_H
#define _intrepid_H

#include <iostream>
#include <cmath>
#include <cfloat>
#include <climits>
#include <vector>
#include <list>

#include "file_utils.h"
#include "general_utils.h"
#include "geometry3d.h"
#include "blocklanguage.h"
#include "stacktrace.h"
#include "logger.h"

class IHeader;
class ILDataset;
class ILField;
class ILSegment;

struct SampleIndex{
	size_t lineindex;
	size_t sampleindex;
};

struct IndexTable{
	size_t start;
	size_t ns;
	size_t dummy1;
	size_t dummy2;
};

struct SortTable{
	double value;
	size_t sequence;
	struct IndexTable index;
};

struct SurveyInfoEntry{
	std::string key;
	std::string value;
};

enum IDataTypeID { dtBYTE, dtSHORT, dtINT, dtFLOAT, dtDOUBLE, dtSTRING, dtUNKNOWN };

class IDatatype {

private:
	
	IDataTypeID  itypeid;
	size_t      bytesize;	

public:

	IDatatype(){
		_GSTITEM_
		itypeid  = dtUNKNOWN;
		bytesize = 0;
	}

	IDatatype(const IDataTypeID tid, const size_t _bytesize = 0){
		_GSTITEM_
		itypeid = tid;
		if (itypeid == dtSTRING) {
			bytesize = _bytesize;
		}
		else bytesize = size();		
	}

	const IDataTypeID getTypeId() const { return  itypeid; }

	const std::string getName() const
	{
		_GSTITEM_
			if (itypeid == dtBYTE) return "UnSigned8BitInteger";
			else if (itypeid == dtSHORT) return "Signed16BitInteger";
			else if (itypeid == dtINT) return "Signed32BitInteger";
			else if (itypeid == dtFLOAT) return "IEEE4ByteReal";
			else if (itypeid == dtDOUBLE) return "IEEE8ByteReal";
			else if (itypeid == dtSTRING) return "String";
			else return "UNKNOWN";
	}

	static unsigned char bytenull() { return 0; }
	static int16_t  shortnull() { return -32767; }
	static int    intnull() { return -2147483647; }
	static float  floatnull() { return -3.4E+38f; }
	static double doublenull() { return -5.0E+75; }
	
	size_t size() const 
	{
		_GSTITEM_
		switch (itypeid) {
		if (itypeid == dtBYTE) return 1;
			case dtSHORT: return 2;
			case dtINT: return 4;
			case dtFLOAT: return 4;
			case dtDOUBLE: return 8;
			case dtSTRING: return bytesize;
			default: glog.logmsg("IDatatype::size() Unknown datatype\n"); return 0;
		}
	}

	bool isbyte() const { if (itypeid == dtBYTE) return true; else return false; }
	bool isshort() const { if (itypeid == dtSHORT) return true; else return false; }
	bool isint() const { if (itypeid == dtINT) return true; else return false; }
	bool isfloat() const { if (itypeid == dtFLOAT) return true; else return false; }
	bool isdouble() const { if (itypeid == dtDOUBLE) return true; else return false;}
	bool isstring() const { if (itypeid == dtSTRING) return true; else return false; }

	double nullasdouble() const {
		_GSTITEM_
		switch (itypeid) {
			case dtBYTE: return (double) bytenull(); break;
			case dtSHORT: return (double) shortnull(); break;
			case dtINT: return (double)intnull(); break;
			case dtFLOAT: return (double)floatnull(); break;
			case dtDOUBLE: return (double)doublenull(); break;
			default: return doublenull();
		}
	}

	static bool isnull(unsigned char number)
	{
		_GSTITEM_
		if (number == bytenull()) return true;
		else return false;
	}	

	static bool isnull(const float number)
	{
		_GSTITEM_
		if (number == floatnull()) return true;
		else if (std::isfinite(number) == false){
			return true;
		}
		return false;
	}

	static bool isnull(const double number)
	{
		_GSTITEM_
		if (number == doublenull()) return true;
		else if (std::isfinite(number) == false){
			return true;
		}
		return false;
	}

	static bool isnull(int16_t number)
	{
		_GSTITEM_
		if (number == shortnull()) return true;
		return false;
	}

	static bool isnull(int number)
	{
		_GSTITEM_
		if (number == intnull()) return true;
		return false;
	}	

};

template<typename T>
class IData{
	
private:
	std::vector<T> buffer;
	
	size_t ns=0;
	size_t nb=0;
	size_t nelements=0;
	size_t ssize=1;//string size for STRINGS
	bool groupby=false;
	
	std::vector<T> _gbbuf;

	std::vector<T> groupbybuffer(){
		std::vector<T> v(ns*nb);
		for (size_t bi = 0; bi < nb; bi++){
			for (size_t si = 0; si < ns; si++){
				v[bi*ns + si] = buffer[bi];
			}
		}
		return v;
	}

public:
	
	IData(){
		ns = 0;
		nb = 0;
		nelements = 0;
		ssize = 1;
		groupby = false;
	}

	void resize(const size_t& _ns, const size_t& _nb = 1, const bool& _groupby = false, const size_t _stringsize = 1)
	{		
		ns = _ns;
		nb = _nb;
		groupby = _groupby;
		ssize = _stringsize;		
		if (groupby){ nelements = nb*ssize; }
		else{ nelements = ns*nb*ssize; }
		buffer.resize(nelements,0);
	}
	
	T& operator()(size_t s, size_t b){
		if (groupby){ return buffer[b*ssize]; }
		else{ return buffer[(s*nb + b)*ssize]; }
	}

	void* pvoid(){ return (void*)buffer.data(); }

	char* pchar(){ return (char*)buffer.data(); }

	void  swap_endian(){
		::swap_endian(buffer);
	}

	void* pvoid_groupby(){
		_gbbuf = groupbybuffer();
		return (void*)_gbbuf.data();		
	}
};

size_t nullindex(){
	return SIZE_MAX;
}

enum eatype  { atDIRECT, atINDEXED };
enum eftype  { ftLINE, ftINDEX, ftPOINT, ftPOLYGON, ftIMAGE};
enum ebptype { bptBSQ, bptBIL, bptBIP };

class IHeader{
	
public:
	bool valid=false;
	eftype filetype;
	eatype accesstype;
	ebptype bandpackingtype;
	size_t nlines=0;
	size_t maxspl=0;
	size_t nbands=0;
	IDatatype datatype;
	size_t headeroffset=0;
	bool endianswap=false;
	std::string indexname;
	
	IHeader(){
		valid = false;
	};

	IHeader(char* p, const std::string& filepath){
		valid = parse(p,filepath);
	}

	bool parse(char* p, const std::string& filepath)
	{		 
		int16_t* s = (int16_t*)p;

		if (s[91] != 1 && s[91] != 2){
			endian_swap(p);			
			if (s[91] != 1 && s[91] != 2){
				return false;
			}
			else endianswap = true;
		}
		else endianswap = false;

		switch (s[72]) {
			case 1000: filetype = ftLINE; break;
			case 1001: filetype = ftIMAGE; printf("filetype ftIMAGE is not supported"); return false;
			case 1002: filetype = ftINDEX; break;
			case 1004: filetype = ftPOLYGON; printf("filetype ftPOLYGON is not supported"); return false;
			case 1006: filetype = ftPOINT; printf("filetype ftPOINT is not supported"); return false;
			default: printf("Bad file type"); return false;
		}

		int16_t dt = s[73];
		int16_t ds = s[90];
		if (dt == 1 && ds == 8)	datatype = IDatatype(dtBYTE);		
		else if (dt == 2 && ds == 16) datatype = IDatatype(dtSHORT);
		else if (dt == 2 && ds == 32) datatype = IDatatype(dtINT);
		else if (dt == 3 && ds == 32) datatype = IDatatype(dtFLOAT);
		else if (dt == 3 && ds == 64) datatype = IDatatype(dtDOUBLE);
		else if (dt == 6) {
			datatype = IDatatype(dtSTRING,ds/8);
		}
		else {
			glog.logmsg("Could not determine datatype from header(dt=%d and ds=%d) in %s\n\n",dt,ds,filepath.c_str());
			return false;
		}
		
		if (filetype==ftINDEX){
			nlines = (size_t)*((int32_t*)&(s[217]));
			maxspl = (size_t)*((int32_t*)&(s[219]));
			nbands = (size_t)*((int32_t*)&(s[221]));
		}
		else if (filetype == ftLINE){
			maxspl = (size_t)*((int32_t*)&(s[217]));
			nlines = (size_t)*((int32_t*)&(s[219]));
			nbands = (size_t)*((int32_t*)&(s[221]));
		}
		
		headeroffset = 512 * (size_t)s[81];		
		switch (s[91]) {
			case 1: accesstype = atDIRECT; break;
			case 2: accesstype = atINDEXED; break;
			default:
				printf("Bad access type");
				return false;
		}

		bandpackingtype = bptBIL;
		if (nbands > 1){
			switch (s[78]) {
				case 0: bandpackingtype = bptBSQ; break;
				case 1: bandpackingtype = bptBIL; break;
				case 2: bandpackingtype = bptBIP; break;
				default:
					if (nbands == 1){
						bandpackingtype = bptBSQ;
					}
					else{
						printf("Bad band packing type");
						return false;
					}
				}
		}	

		char tmp[26];
		strncpy(tmp,(char*)&s[171],25);
		tmp[25] = 0;
		indexname = std::string(tmp);
				
		//printf("proj = %d\n",(int)s[92]);
		//printf("???? = %d\n", (int)s[93]);
		return true;
	};

	void endian_swap(void* p)
	{
		int16_t* s = (int16_t*)p;
		swap_endian((int32_t*)&s[10], 1);
		swap_endian((int32_t*)&s[12], 1);
		swap_endian((int16_t*)&s[32], 1);
		swap_endian((int16_t*)&s[33], 1);
		swap_endian((int16_t*)&s[34], 1);
		swap_endian((int16_t*)&s[72], 1);
		swap_endian((int16_t*)&s[73], 1);
		swap_endian((int16_t*)&s[78], 1);
		swap_endian((int16_t*)&s[79], 1);
		swap_endian((int16_t*)&s[81], 1);
		swap_endian((int16_t*)&s[87], 1);
		swap_endian((int16_t*)&s[89], 1);
		swap_endian((int16_t*)&s[90], 1);
		swap_endian((int16_t*)&s[91], 1);
		swap_endian((int16_t*)&s[92], 1);
		swap_endian((int16_t*)&s[93], 1);
		swap_endian((int32_t*)&s[115], 1);
		swap_endian((int32_t*)&s[117], 1);
		swap_endian((double*)&s[119], 1);
		swap_endian((double*)&s[123], 1);
		swap_endian((double*)&s[131], 1);
		swap_endian((double*)&s[135], 1);
		swap_endian((double*)&s[143], 1);
		swap_endian((double*)&s[147], 1);
		swap_endian((int16_t*)&s[188], 1);
		swap_endian((int16_t*)&s[189], 1);
		swap_endian((int32_t*)&s[217], 1);
		swap_endian((int32_t*)&s[219], 1);
		swap_endian((int32_t*)&s[221], 1);
	};
	static size_t nbytes() { return 512; }
};

class ILSegment{

private:
	ILField&  Field;
	const size_t lineindex;

public:
			
	IData<float> fdata;
	IData<double> ddata;
	IData<int32_t> idata;
	IData<int16_t> sdata;
	IData<char> cdata;
	IData<char> strdata;
	
	bool readbuffer();
	bool writebuffer();
	
	const ILField& getField() const;
	const ILDataset& getDataset() const;	
	const size_t& startindex() const;
	const size_t& nlines() const;
	const size_t& nsamples() const;
	const size_t& nbands() const;
	const IDatatype& getType();
	const IDataTypeID& getTypeId();
	FILE* filepointer();

	ILSegment(ILField& _field, size_t _lineindex) : Field(_field), lineindex(_lineindex) {};

	void createbuffer()
	{
		size_t ns = nsamples();
		size_t nb = nbands();
		bool groupby = isgroupbyline();
		
		switch (getTypeId()){
			case dtFLOAT: fdata.resize(ns,nb,groupby); return;
			case dtDOUBLE: ddata.resize(ns, nb, groupby); return;
			case dtSHORT: sdata.resize(ns, nb, groupby); return;
			case dtINT: idata.resize(ns, nb, groupby); return;
			case dtBYTE: cdata.resize(ns, nb, groupby); return;
			case dtSTRING: strdata.resize(ns, nb, groupby); return;
			default: printf("ILSegment::createbuffer() Unknown type");
		}
	}

	void* pvoid()
	{
		switch (getTypeId()){
			case dtFLOAT: return fdata.pvoid();
			case dtDOUBLE: return ddata.pvoid();
			case dtSHORT: return sdata.pvoid();
			case dtINT: return idata.pvoid();
			case dtBYTE: return cdata.pvoid();
			case dtSTRING: return sdata.pvoid();
			default: printf("ILSegment::pvoid() Unknown type"); return (void*)NULL;
		}
	}

	void* pvoid_groupby()
	{		
		switch (getTypeId()){
			case dtFLOAT: return fdata.pvoid_groupby();
			case dtDOUBLE: return ddata.pvoid_groupby();
			case dtSHORT: return sdata.pvoid_groupby();
			case dtINT: return idata.pvoid_groupby();
			case dtBYTE: return cdata.pvoid_groupby();
			case dtSTRING: return strdata.pvoid_groupby();
			default: printf("ILSegment::pvoid_groupby() Unknown type"); return (void*)NULL;
		}
	}

	double d(size_t s, size_t b = 0){		
		
		switch (getTypeId()){
			case dtFLOAT:{
				if (IDatatype::isnull(fdata(s, b))) return IDatatype::doublenull();
				else return (double)fdata(s, b);
			}
			case dtDOUBLE: return ddata(s, b);
			case dtSHORT:{
				if (IDatatype::isnull(sdata(s, b))) return IDatatype::doublenull();
				else return (double)sdata(s, b);
			}
			case dtINT:{
				if (IDatatype::isnull(idata(s, b))) return IDatatype::doublenull();
				else return (double)idata(s, b);
			}
			case dtBYTE:{
				if (IDatatype::isnull(cdata(s, b))) return IDatatype::doublenull();
				else return (double)cdata(s, b);
			}
			default:{
				printf("ILSegment::d() Unknown type"); return IDatatype::doublenull();
			}
		}
	}

	float f(size_t s, size_t b = 0)
	{
		switch (getTypeId()){
			case dtFLOAT: return fdata(s, b); break;
			case dtDOUBLE: return (float)ddata(s, b); break;
			case dtSHORT: return (float)sdata(s, b); break;
			case dtINT: return (float)idata(s, b); break;
			case dtBYTE: return (float)cdata(s, b); break;
			default: printf("ILSegment::f() Unknown type"); return IDatatype::floatnull();
		}
	}

	int32_t i(size_t s, size_t b = 0)
	{
		switch (getTypeId()){
			case dtFLOAT: return (int32_t)fdata(s, b);
			case dtDOUBLE: return (int32_t)ddata(s, b);
			case dtSHORT: return (int32_t)sdata(s, b);
			case dtINT: return idata(s, b);
			case dtBYTE: return (int32_t)cdata(s, b);
			default: printf("ILSegment::i() Unknown type"); return IDatatype::intnull();
		}
	}

	int16_t s(size_t s, size_t b = 0)
	{
		switch (getTypeId()){
			case dtFLOAT: return (int16_t)fdata(s, b);
			case dtDOUBLE: return (int16_t)ddata(s, b);
			case dtSHORT: return sdata(s, b);
			case dtINT: return (int16_t)idata(s, b);
			case dtBYTE: return (int16_t)cdata(s, b);
			default: printf("ILSegment::s() Unknown type"); return IDatatype::shortnull();
		}
	}

	
	template <typename T>
	bool getband(std::vector<T>& v, size_t band=0)
	{
		size_t ns = nsamples();
		if (isgroupbyline()) ns = 1;		
		v.resize(ns);

		if (getTypeId() == dtSTRING) {
			size_t len = getType().size();
			char* p = strdata.pchar();						
			for (size_t i = 0; i < ns; i++) {				
				std::string s(p,len);				
				str2num(s,v[i]);
				//std::stringstream ss(p);
				//ss >> v[i];
				p += len;
			}
			return true;
		}
				
		switch (getTypeId()){
			case dtFLOAT: 			
				for (size_t i = 0; i< ns; i++){				
					v[i] = (T)fdata(i, band);
				}		
				return true;
			case dtDOUBLE: 
				for (size_t i = 0; i< ns; i++){				
					v[i] = (T)ddata(i, band);
				}
				return true;
			case dtSHORT:
				for (size_t i = 0; i< ns; i++){				
					v[i] = (T)sdata(i, band);					
				}
				return true;
			case dtINT:
				for (size_t i = 0; i< ns; i++){				
					v[i] = (T)idata(i, band);
				}
				return true;
			case dtBYTE:
				for (size_t i = 0; i< ns; i++){				
					v[i] = (T)cdata(i, band);
				}			
				return true;						
			default: printf("ILSegment::i() Unknown type"); return false;
		}
	}

	size_t nstored(){
		if (isindexed())return nsamples();
		else return 1;
	}
	
	size_t nelements(){
		return nstored()*nbands();
	}
	
	size_t nbytes(){
		size_t nb = nelements() * getType().size();
		return nb;
	}

	bool isgroupbyline();

	bool isindexed();

	long fileposition()
	{
		if (isgroupbyline()){
			long p = (long)(IHeader::nbytes() + lineindex * nbands() * getType().size());
			return p;
		}
		else{
			long p = (long)(IHeader::nbytes() + startindex() * nbands() * getType().size());
			return p;
		}
	}
	
	template<typename T>
	void change_nullvalue(const T& newnullvalue) {
		_GSTITEM_
		const IDatatype& dt = getType();
		if(dt.isnull(newnullvalue)) return;
		T* fp = (T*)pvoid();
		for (size_t k = 0; k < nstored(); k++) {
			if (dt.isnull(fp[k])) {
				fp[k] = newnullvalue;
			}
		}		
	}

	/*
	template<typename T>
	bool write(const std::vector<std::vector<T>>& array)
	{
		switch (datatype().etype()){
		case dtFLOAT: array2ptr((float*)buffer.data(), array); break;
		case dtDOUBLE: ptr2array((double*)buffer.data(), array); break;
		case dtSHORT: ptr2array((int16_t*)buffer.data(), array); break;
		case dtINT: ptr2array((int32_t*)buffer.data(), array); break;
		case dtBYTE: ptr2array((char*)buffer.data(), array); break;
		default: printf("ILSegment::write() Unknown type"); return false;
		}
		std::vector<char> buffer = array2ptr(array);
		return writebuffer();
	}*/

};

class ILField{

private:		

	ILDataset& Dataset;
	IHeader Header;	
	FILE* pFile;
	std::string Name;

public:	
		
	std::string Datum;
	std::string Projection;
	std::string CoordinateType;
	
	const ILDataset& getDataset() const { return Dataset; }
	const std::string& getName() const { return Name; }
	const IDatatype& getType() const { return Header.datatype; }
	const IDataTypeID&  getTypeId() const { return Header.datatype.getTypeId(); }
	const size_t& nbands() const { return Header.nbands; };	
	const size_t& nlines() const;
	FILE* filepointer() { return pFile; }	
	const bool& endianswap() const { return Header.endianswap; }
	
	const bool& isgroupbyline() const
	{
		if (Header.accesstype == atDIRECT) return true; 			
		else return false;
	}
	const bool& isindexed() const
	{
		if (Header.accesstype == atINDEXED) return true;
		else return false;
	}
	
	ILField();

	ILField(ILDataset& dataset, const std::string& fieldname) 
		: Dataset(dataset)
	{				
		initialise_existing(fieldname);
	}

	ILField(ILDataset& dataset, const std::string& _fieldname, IDatatype _datatype, size_t _nbands, bool _indexed) 
		: Dataset(dataset)
	{
		create_new(_fieldname,_datatype,_nbands,_indexed);
	}
		
	~ILField() { close(); }

	ILDataset& getDataset() { return Dataset; }
	
	bool isvalid()
	{
		if (datafilepath().size() > 0)return true;
		return false;
	}

	bool initialise_existing(const std::string& fieldname);
	
	bool create_new(const std::string& fieldname, const IDatatype& datatype, const size_t& nbands, const bool& indexed);

	bool open()
	{
		if (pFile != (FILE*)NULL)return true;
		if ((pFile = fileopen(datafilepath(), "rb")) == NULL) {
			glog.logmsg("ILField::open() cannot open file: %s\n\n", datafilepath().c_str());
			return false;
		}

		std::vector<char> buffer(IHeader::nbytes());
		fread(buffer.data(), IHeader::nbytes(), 1, pFile);
		Header = IHeader(buffer.data(),datafilepath());
		if (Header.valid==false){
			glog.logmsg("Could not read header in file: %s\n\n", datafilepath().c_str());
			fclose(pFile);
			pFile = (FILE*)NULL;
			return false;
		}		
		return true;
	}	
	
	void close()
	{
		if (pFile){
			fclose(pFile);
		}
		pFile = (FILE*)NULL;
	}
	
	bool erase()
	{
		bool status = true;
		close();
		if (remove(datafilepath().c_str()) != 0){
			if (errno == EACCES)status = false;
		}

		if (remove(dotvecfilepath().c_str()) != 0){
			if (errno == EACCES)status = false;
		}

		if (remove(dotdotlinefilepath().c_str()) != 0){
			if (errno == EACCES)status = false;
		}
		return status;
	}

	std::string infostring(){
		std::string s;
		s += strprint(Name.c_str());
		s += strprint(" Type=%s ", Header.datatype.getName().c_str());
		s += strprint(" Bands=%lu ", Header.nbands);
		if (isgroupbyline())s += strprint(" GroupBy ");
		if (isindexed())s += strprint(" Indexed ");
		s += strprint("\n");
		return s;
	}

	void printinfo()
	{
		std::string s = infostring();
		std::printf(s.c_str());
	}
	
	const std::string& datasetpath() const ;
	
	const std::string datafilepath() const 
	{
		return datasetpath() + Name + ".PD";		
	}

	const std::string dotvecfilepath() const 
	{
		return datasetpath() + Name + ".PD.vec";		
	}

	const std::string dotdotlinefilepath() const 
	{
		return datasetpath() + Name + "..LINE";		
	}
	
	const size_t groupbyindex(const int value)
	{
		for (size_t li = 0; li < nlines(); li++) {
			ILSegment s(*this, li);
			int v = s.i(0, 0);
			if (v == value) return li;
		}
		return nullindex();
	}

	bool parse_datum_projection()
	{
		cBlock b(dotvecfilepath());				
		if (b.Entries.size() > 0) {
			cBlock c = b.findblock("CoordinateSpace");
			if (c.Entries.size() > 0) {
				std::string str;								
				if (c.getvalue("Datum", str)) Datum = stripquotes(str);				
				if (c.getvalue("Projection", str)) Projection = stripquotes(str);
				if (c.getvalue("CoordinateType", str)) CoordinateType = stripquotes(str);
			}
		}
		return false;
	}
	
};

class ILDataset{

private:	
	IHeader Header;
	std::vector<SurveyInfoEntry> SurveyInfo;
	
	bool readsurveyinfo()
	{
		_GSTITEM_
		FILE* fsurveyinfo;
		if ((fsurveyinfo = fileopen(surveyinfopath, "r")) == NULL) {
			glog.logmsg("Cannot open file: %s\n\n", surveyinfopath.c_str());
			return false;
		}

		std::string lstr;
		while (filegetline(fsurveyinfo, lstr)){
			size_t epos = lstr.find_first_of("=");
			if (epos<lstr.size()){
				lstr = trim(lstr);
				size_t len = lstr.size();
				epos = lstr.find_first_of("=");
				std::string lhs = lstr.substr(0, epos);
				std::string rhs = lstr.substr(epos + 1, len - epos);
				lhs = trim(lhs);
				rhs = trim(rhs);

				SurveyInfoEntry e;
				e.key = lhs;
				e.value = rhs;
				SurveyInfo.push_back(e);				
			}
		}
		fclose(fsurveyinfo);
		return true;
	}

	bool getfields()
	{
		_GSTITEM_		
		std::vector<std::string> fall = getfilelist(datasetpath, "");
		std::vector<std::string> filelist;
		for (size_t i = 0; i < fall.size(); i++) {
			std::string e = extractfileextension(fall[i]);
			if (strcasecmp(e, ".PD") == 0){
				//Needs to be done this way because of case sensitivity on linux
				filelist.push_back(fall[i]);
			}
		}

		//Count valid fields		
		for (size_t i = 0; i<filelist.size(); i++){			
			std::string name = extractfilename_noextension(filelist[i]);
			std::string ext  = extractfileextension(filelist[i]);

			if (strcasecmp(name, "INDEX") == 0){
				continue;
			}

			if (strcasecmp(ext.c_str(),".PD")==0){				
				Fields.push_back(ILField(*this,name));
			}
		}		
		return true;
	}

public:
	bool valid;
	std::string datasetpath;
	std::string surveyinfopath;
	std::string indexpath;
	
	std::list<ILField> Fields;
	
	ILDataset() {};

	ILDataset(const std::string& _datasetpath)
	{
		_GSTITEM_
		valid = false;

		datasetpath = strippath(_datasetpath);		
		if (datasetpath.size() <= 0){
			glog.logmsg("ILDataset: invalid dataset path: %s\n\n", datasetpath.c_str());
			valid = false;
			return;
		}
		
		std::string i1 = datasetpath + "INDEX.PD";
		std::string i2 = datasetpath + "INDEX.pd";
		std::string i3 = datasetpath + "index.PD";
		std::string i4 = datasetpath + "index.pd";
		
		if(exists(i1)) indexpath = i1;
		else if(exists(i2)) indexpath = i2;
		else if(exists(i3)) indexpath = i3;
		else if(exists(i4)) indexpath = i4;
		else {
			glog.logmsg("ILDataset: cannot find an index file for dataset %s\n\n", datasetpath.c_str());
			valid = false;
			return;
		};

		FILE* findex = fileopen(indexpath, "rb");
		if (findex == NULL) {
			glog.logmsg("ILDataset: cannot open file %s\n\n", indexpath.c_str());
			valid = false;
			return;
		}
		
		surveyinfopath = datasetpath + "SurveyInfo";
		if (readsurveyinfo() == false){
			valid = false;
			return;
		}

		std::vector<char> buffer(IHeader::nbytes());		
		fread(buffer.data(), IHeader::nbytes(), 1, findex);		
		Header = IHeader(buffer.data(),indexpath);
		if (Header.valid==false){
			glog.logmsg("ILDataset: could not read INDEX file: %s\n\n", indexpath.c_str());
			valid = false;
			return;
		}
		else if (Header.filetype != ftINDEX){		
			glog.logmsg("ILDataset: file %s is not an INDEX file\n\n", indexpath.c_str());
			valid = false;
			return;
		}

		if (ispointdataset()) {
			//Currently not supporting point databases
			valid = false;
			return;
		}

		indextable.resize(nlines());		
		std::vector<int> indexdata(nlines() * 4);		
		if (fread((void*)indexdata.data(), 16, nlines(), findex) != (size_t)nlines()){
			glog.logmsg("ILDataset Error reading INDEX file: %s\n\n", indexpath.c_str());
			valid = false;
			return;
		}
		
		if(Header.endianswap){
			swap_endian(indexdata.data(), nlines() * 4);
		}

		for (size_t i = 0; i<nlines(); i++){
			indextable[i].start = (size_t)indexdata[i * 4];
			indextable[i].ns = (size_t)indexdata[i * 4 + 1];
			indextable[i].dummy1 = (size_t)indexdata[i * 4 + 2];
			indextable[i].dummy2 = (size_t)indexdata[i * 4 + 3];
			if (indextable[i].ns > Header.maxspl){
				Header.maxspl = indextable[i].ns;
			}
		}				
		fclose(findex);

		if (getfields() == true){
			valid = true;
		}
		return;
	}

	~ILDataset()
	{
		_GSTITEM_
	}

	std::vector<IndexTable> indextable;
	std::vector<cLineSeg> bestfitlinesegs;
	
	static std::string dbdirpath(const std::string& path){
		_GSTITEM_
		std::string ext = extractfileextension(path);
		size_t pos = path.find("..DIR");
		std::string db = path.substr(0, pos);
		return db;
	}

	static std::string dbname(const std::string& path){				
		_GSTITEM_
		std::string dp = dbdirpath(path);
		std::string dn = extractfilename(dp);
		return dn;
	}

	bool ispointdataset() {
		if (maxspl() == 1 && nlines() == 1) return true;
		return false; 
	}

	const size_t& nlines() const { return Header.nlines; }
	const size_t& maxspl() const {return Header.maxspl;	}

	const size_t& nfields() const { return Fields.size(); }			
	const size_t& nsamplesinline(const size_t segindex) const {
		return indextable[segindex].ns;
	}
	const size_t& nsamples() const {
		_GSTITEM_
		size_t n = 0;
		for (size_t li = 0; li < nlines(); li++){
			n += nsamplesinline(li);
		}
		return n;
	}

	const size_t& startindex(const size_t segindex) const {
		return indextable[segindex].start;
	}
	
	std::vector<size_t> linesamplecount(){
		std::vector<size_t> count(nlines());
		for (size_t i = 0; i < nlines(); i++){
			count[i] = indextable[i].ns;
		}
		return count;
	}

	static std::string strippath(const std::string& path)
	{
		_GSTITEM_
		std::string p = path;
		fixseparator(p);

		size_t len = p.size();
		if (len >= 5){
			if (p.substr(len - 5, 5) == "..DIR"){
				p.resize(len - 5);;
			}
			else if (p.substr(len - 5, 5) == "..dir"){
				p.resize(len - 5);
			}
		}

		len = p.size();
		if (len >= 1){
			if (p[len - 1] != pathseparator()){
				p += pathseparatorstring();
			}
		}
		return p;
	}	
	
	static bool isdatabase(const std::string& path)
	{
		_GSTITEM_
		std::string p = strippath(path);
		size_t len = p.size();
		if (len <= 1) return false;

		std::string indexfile = p + "INDEX.PD";
		bool isvalid = exists(indexfile);
		if (isvalid)return true;

		indexfile = p + "index.pd";
		isvalid = exists(indexfile);
		if (isvalid)return true;
		else return false;
	}

	std::string infostring()
	{
		_GSTITEM_
		std::string s;
		s += strprint("Dataset Information\n");
		s += strprint(datasetpath.c_str());		
		s += strprint(indexpath.c_str());		
		s += strprint(surveyinfopath.c_str());		
		s += strprint("%lu Lines\n", nlines());
		s += strprint("Maximum samples per line = %lu\n\n", maxspl());
		s += strprint("Fields %lu\n", nfields());
		for (auto it = Fields.begin(); it != Fields.end(); ++it){
			s += "\t";
			s += it->infostring();
		}

		if (valid == true){
			s += strprint("Valid = Yes\n");
		}
		else{
			s += strprint("Valid = No\n");
		}
		return s;
	}

	void printinfo()
	{
		_GSTITEM_
		std::string s = infostring();
		std::printf(s.c_str());		
	}

	bool fieldexists_ignorecase(const std::string& fieldname)
	{
		_GSTITEM_
		for (auto it = Fields.begin(); it != Fields.end(); ++it){
			if (strcasecmp(it->getName().c_str(),fieldname.c_str())==0){
				return true;
			}
		}
		return false;
	}

	bool fieldexists(const std::string& fieldname)
	{
		_GSTITEM_
		for (auto it = Fields.begin(); it != Fields.end(); ++it) {
			if (strcmp(it->getName().c_str(), fieldname.c_str()) == 0) {
				return true;
			}
		}
		return false;
	}

	ILField& getfield(const std::string& fieldname)
	{
		_GSTITEM_
		for (auto it = Fields.begin(); it != Fields.end(); ++it){			
			if (strcasecmp(it->getName().c_str(), fieldname.c_str()) == 0){
				return *it;
			}
		}	
		return ILField();
	}

	ILField& getsurveyinfofield(const std::string& key)
	{
		_GSTITEM_
		std::string fieldname;
		bool status = surveyinfofieldname(key,fieldname);
		if (status){
			printf("Cannot find field %s from SurveyInfo:\n\n", key.c_str());
			return ILField();
		}
		return getfield(fieldname);
	}	
	
	bool erasefield(const std::string& fieldname)
	{
		_GSTITEM_
		for (auto it = Fields.begin(); it != Fields.end(); ++it){
			if (strcasecmp(it->getName().c_str(), fieldname.c_str()) == 0){
				bool status = it->erase();
				if (status){
					Fields.erase(it);
				}
				return status;
			}
		}
		return false;			
	}
	
	bool addfield(const std::string& fieldname, IDatatype datatype, size_t nbands=1, bool isindexed=true)
	{
		_GSTITEM_
		if (fieldexists_ignorecase(fieldname)){
			printf("ILDataset::addfield() %s already exists\n\n", fieldname.c_str());
			return false;
		}
		Fields.push_back(ILField(*this, fieldname, datatype, nbands, isindexed));

		ILField& F = getfield(fieldname);
		for (size_t li = 0; li < nlines(); li++){
			ILSegment S(F,li);
			S.createbuffer();
			S.writebuffer();
		}

		return true;
	}
	
	std::string fieldnamelike(const std::string& identifer) const
	{
		_GSTITEM_
		for (auto it = Fields.begin(); it != Fields.end(); ++it){			
			if (strcasecmp(it->getName(), identifer) == 0){
				return it->getName();
			}
		}		
		//printf("Cannot find field like: identifer %s\n\n", identifer.c_str());
		return "";
	}

	bool hassurveyinfokey(const std::string& key)
	{
		_GSTITEM_
		for (size_t i = 0; i < SurveyInfo.size(); i++){
			if (strcasecmp(SurveyInfo[i].key, key) == 0){
				return true;
			}
		}
		return false;		
	}

	bool hassurveyinfokey_and_fieldexists(const std::string& key)
	{
		_GSTITEM_
		if(hassurveyinfokey(key)){
			std::string fname;
			bool status = surveyinfofieldname(key,fname);
			if(status) return fieldexists_ignorecase(fname);
		}			
		return false;
	}

	bool surveyinfofieldname(const std::string& key, std::string& fieldname)
	{
		_GSTITEM_
		fieldname = std::string();
		for (size_t i = 0; i < SurveyInfo.size(); i++){
			if (strcasecmp(SurveyInfo[i].key, key) == 0){
				fieldname = SurveyInfo[i].value;
				return true;
			}
		}		
		return false;
	}

	bool fieldalias(const std::string& fieldname, std::string& key) const
	{
		_GSTITEM_
		key = std::string();
		for (size_t i = 0; i < SurveyInfo.size(); i++) {
			if (strcasecmp(SurveyInfo[i].value, fieldname) == 0) {
				key = SurveyInfo[i].key;
				return true;
			}
		}
		return false;
	}

	void bestfitlines()
	{
		_GSTITEM_
		if (bestfitlinesegs.size() > 0)return;
		
		ILField& fX = getsurveyinfofield("X");
		ILField& fY = getsurveyinfofield("Y");

		for (size_t li = 0; li<nlines(); li++){
			ILSegment sX(fX,li);
			ILSegment sY(fY,li);
			size_t numsamples = sX.nsamples();

			////Find first and last non nulls
			size_t s = 0;
			size_t firstnonnull, lastnonnull;
			double xv, yv;
			do{
				xv = sX.d(s);
				yv = sY.d(s);
				if (IDatatype::isnull(xv) || IDatatype::isnull(yv)){
					s++;
				}
				else{
					firstnonnull = s;
					break;
				}
			} while (s<numsamples);

			s = numsamples - 1;
			do{
				xv = sX.d(s);
				yv = sY.d(s);
				if (IDatatype::isnull(xv) || IDatatype::isnull(yv)){
					s--;
				}
				else{
					lastnonnull = s;
					break;
				}
			} while (s>0);


			///////////////////
			size_t n = 40;
			double x[40];
			double y[40];

			size_t validsamples = lastnonnull - firstnonnull + 1;
			if (n>validsamples)n = validsamples;
			size_t di = validsamples / n;
			for (s = 0; s<n; s++){
				x[s] = sX.d(firstnonnull + s*di);
				y[s] = sY.d(firstnonnull + s*di);
			}

			cPnt p1, p2;
			cLineSeg seg;
			double gradient, intercept;
			if (fabs(x[0] - x[n - 1]) > fabs(y[0] - y[n - 1])){
				regression(x, y, n, &gradient, &intercept);
				p1.x = sX.d(firstnonnull);
				p2.x = sX.d(lastnonnull);
				p1.y = gradient * p1.x + intercept;
				p2.y = gradient * p2.x + intercept;
			}
			else{
				regression(y, x, n, &gradient, &intercept);
				p1.y = sY.d(firstnonnull);
				p2.y = sY.d(lastnonnull);
				p1.x = gradient * p1.y + intercept;
				p2.x = gradient * p2.y + intercept;
			}
			p1.z = 0.0;
			p2.z = 0.0;

			seg.set(p1, p2);
			bestfitlinesegs.insert(bestfitlinesegs.end(), seg);
		}
		fX.close();
		fY.close();
	}
	
	
	bool getlinenumberfieldname(std::string& fieldname) {
		if (surveyinfofieldname("LineNumber", fieldname)){ return true; }
		else if (fieldexists("LINE")){ fieldname = "LINE"; return true; }
		else if (fieldexists("Line")) { fieldname = "Line"; return true; }
		else if (fieldexists("line")) { fieldname = "line"; return true; }
		else return false;
	}

	template <typename T>
	bool getlinenumbers(std::vector<T>& v){		
		std::string fieldname;
		bool status = getlinenumberfieldname(fieldname);
		if (fieldexists(fieldname)){
			ILField& F  = getfield(fieldname);
			bool status = getgroupbydata(F, v);
			return status;
		}		
		return false;
	}

	template <typename T> 
	bool getgroupbydata(ILField& F, std::vector<T>& v, const size_t band=0){
		v.resize(nlines());
		for (size_t li = 0; li < nlines(); li++){
			ILSegment S(F,li);
			std::vector<T> b;
			if (S.readbuffer()) {			
				S.getband(b, band);
				v[li] = b[0];
			}
			else {
				return false;
			}
		}
		return true;
	}

	double distancetobestfitline(cPnt p, size_t i)
	{
		_GSTITEM_
		bestfitlines();
		cPnt c = bestfitlinesegs[i].closestpoint(p);
		double d = c.distance(p);
		return d;
	}

	size_t nearestbestfitline(cPnt p)
	{
		_GSTITEM_
		bestfitlines();
		size_t index = 0;
		double mindistance, d;
		for (size_t li = 0; li<nlines(); li++){
			d = distancetobestfitline(p, li);
			if (li == 0)mindistance = d;
			if (d<mindistance){
				mindistance = d;
				index = li;
			}
		}
		return index;
	}

	double nearestsample(const cPnt p, size_t& lineindex, size_t& sampleindex, double& x, double& y)
	{
		_GSTITEM_
		lineindex = nearestbestfitline(p);
		sampleindex = 0;

		double mindistance;

		ILField& fX = getsurveyinfofield("X");
		ILField& fY = getsurveyinfofield("Y");

		ILSegment sX(fX,lineindex);
		ILSegment sY(fY,lineindex);

		for (size_t si = 0; si<sX.nsamples(); si++){
			double dx = p.x - sX.d(si);
			double dy = p.y - sY.d(si);
			double d = sqrt(dx*dx + dy*dy);
			if (si == 0)mindistance = d;
			if (d <= mindistance){
				mindistance = d;
				sampleindex = si;
				x = sX.d(si);
				y = sY.d(si);
			}
		}
		return mindistance;
	}

	std::vector<SampleIndex> sampleswithindistance(cPnt p, double distance)
	{
		_GSTITEM_
		bestfitlines();
		std::vector<SampleIndex> samples;

		ILField& fX = getsurveyinfofield("X");
		ILField& fY = getsurveyinfofield("Y");

		for (size_t li = 0; li<nlines(); li++){
			double d = distancetobestfitline(p, li);
			if (d<distance*2.0){
				ILSegment sX(fX,li);
				ILSegment sY(fY,li);
				size_t nsam = sX.nsamples();
				for (size_t si = 0; si<nsam; si++){
					cPnt p1(sX.d(si), sY.d(si), 0.0);
					if (p.distance(p1) <= distance){
						struct SampleIndex sam;
						sam.lineindex = li;
						sam.sampleindex = si;
						samples.push_back(sam);
					}
				}
			}
		}
		fX.close();
		fY.close();
		return samples;
	}

	SampleIndex linefid_index(int linenumber, int fidnumber)
	{
		_GSTITEM_
		ILField& fLine = getsurveyinfofield("LineNumber");
		ILField& fFid  = getsurveyinfofield("Fiducial");

		SampleIndex sindex;
		sindex.lineindex = fLine.groupbyindex(linenumber);
		if (sindex.lineindex == nullindex())return sindex;
		
		ILSegment sFid(fFid,sindex.lineindex);
		sindex.sampleindex = nullindex();
		for (size_t si = 0; si<sFid.nsamples(); si++){
			if (sFid.i(si) == fidnumber){
				sindex.sampleindex = si;
				break;
			}
		}
		return sindex;
	}

	cStats<double> fieldstats(const std::string& fieldname){
		_GSTITEM_
		ILField& F = getfield(fieldname);
		std::vector<double> v;
		v.reserve(nsamples());		
		for (size_t li = 0; li < nlines(); li++){
			ILSegment S(F,li);			
			S.readbuffer();	
			size_t nsamples = S.nsamples();			
			for (size_t si = 0; si < nsamples; si++){				
				double val = S.d(si);
				if (IDatatype::isnull(val)==false){
					v.push_back(val);					
				}				
			}
		}				
		cStats<double> stats(v);
		return stats;		
	}
	
	template<typename T>
	void getdata(const std::string& fieldname, std::vector<T> v){
		_GSTITEM_

		ILField& F = *getfield(fieldname);		
		v.reserve(nsamples());
		for (size_t li = 0; li < nlines(); li++){
			ILSegment& S = F.Segments[li];
			S.readbuffer();
			F.getType();
			size_t nsamples = S.nsamples();
			for (size_t si = 0; si < nsamples; si++){
				T val = S.d(si);
				v.push_back(val);				
			}
		}		
	}

	bool get_line_start_end_points(std::vector<double>& x1, std::vector<double>& x2, std::vector<double>& y1, std::vector<double>& y2)
	{
		_GSTITEM_
		ILField& fx = getsurveyinfofield("X");
		ILField& fy = getsurveyinfofield("Y");

		size_t nl = nlines();

		IDatatype dt = fx.getType();
		
		x1.resize(nl);
		y1.resize(nl);
		x2.resize(nl);
		y2.resize(nl);
		
		for (size_t li = 0; li < nl; li++){
			ILSegment sx(fx,li);
			ILSegment sy(fy,li);
			sx.readbuffer();
			sy.readbuffer();
			size_t ns = sx.nsamples();
			for (size_t k = 0; k<ns; k++){
				x1[li] = sx.d(k);
				y1[li] = sy.d(k);
				if (dt.isnull(x1[li]) == false && dt.isnull(y1[li]) == false)break;
			}

			for (size_t k = ns - 1; k != 0; k--){
				x2[li] = sx.d(k);
				y2[li] = sy.d(k);
				if (dt.isnull(x2[li]) == false && dt.isnull(y2[li]) == false)break;
			}
		}		
		return true;
	}
};


///////////////////////////////////////////////////////////////////

ILField::ILField() : Dataset(ILDataset()) {	};

const ILField& ILSegment::getField() const { return Field; }

const ILDataset& ILSegment::getDataset() const { return Field.getDataset(); }

const size_t& ILSegment::startindex() const { return getDataset().startindex(lineindex); }

const size_t& ILSegment::nlines() const { return getDataset().nlines(); }

const size_t& ILSegment::nsamples() const { return getDataset().nsamplesinline(lineindex); }

const size_t& ILSegment::nbands() const { return Field.nbands(); }

const IDatatype& ILSegment::getType() { return Field.getType(); }

const IDataTypeID& ILSegment::getTypeId() { return Field.getTypeId(); }

FILE* ILSegment::filepointer() { return Field.filepointer(); }

bool ILSegment::isgroupbyline()
{
	return Field.isgroupbyline();
}

bool ILSegment::isindexed()
{
	return Field.isindexed(); 
}

bool ILSegment::readbuffer()
{
	size_t n;
	bool status = Field.open();
	if (status == false){
		return false;
	}

	long move = fileposition() - std::ftell(filepointer());
	std::fseek(filepointer(), move, SEEK_CUR);
	const size_t len = getType().size();

	switch (getTypeId()){
	case dtFLOAT:
		fdata.resize(nsamples(), nbands(), isgroupbyline());
		n = std::fread(fdata.pvoid(), nbytes(), 1, filepointer());
		if(Field.endianswap()) fdata.swap_endian();
		break;
	case dtDOUBLE:
		ddata.resize(nsamples(), nbands(), isgroupbyline());
		n = std::fread(ddata.pvoid(), nbytes(), 1, filepointer());
		if (Field.endianswap()) ddata.swap_endian();
		break;
	case dtSHORT:
		sdata.resize(nsamples(), nbands(), isgroupbyline());
		n = std::fread(sdata.pvoid(), nbytes(), 1, filepointer());
		if (Field.endianswap()) sdata.swap_endian();
		break;
	case dtINT:
		idata.resize(nsamples(), nbands(), isgroupbyline());
		n = std::fread(idata.pvoid(), nbytes(), 1, filepointer());
		if (Field.endianswap()){
			idata.swap_endian();
		}
		break;
	case dtBYTE:
		cdata.resize(nsamples(), nbands(), isgroupbyline() );
		n = std::fread(cdata.pvoid(), nbytes(), 1, filepointer());
		if (Field.endianswap()) cdata.swap_endian();
		break;
	case dtSTRING:		
		strdata.resize(nsamples(), nbands(), isgroupbyline(), len);
		n = std::fread(strdata.pvoid(), nbytes(), 1, filepointer());
		if (Field.endianswap()) strdata.swap_endian();
		break;
	default: std::printf("ILSegment::read() Unknown type"); return false;
	}
		
	if (n != 1){
		std::printf("ILSegment::readbuffer Error reading file %s\n", Field.datafilepath().c_str());
		return false;
	}	
	return true;
}

bool ILSegment::writebuffer()
{
	size_t n;
	Field.open();
	long move = fileposition() - ftell(filepointer());
	fseek(filepointer(), move, SEEK_CUR);

	switch (getTypeId()){
	case dtFLOAT:
		if (Field.endianswap()) fdata.swap_endian();				
		n = fwrite(fdata.pvoid(), nbytes(), 1, filepointer());
		break;
	case dtDOUBLE:
		if (Field.endianswap()) ddata.swap_endian();		
		n = fwrite(ddata.pvoid(), nbytes(), 1, filepointer());		
		break;
	case dtSHORT:
		if (Field.endianswap()) sdata.swap_endian();
		n = fwrite(sdata.pvoid(), nbytes(), 1, filepointer());		
		break;
	case dtINT:
		if (Field.endianswap()) idata.swap_endian();
		n = fwrite(idata.pvoid(), nbytes(), 1, filepointer());		
		break;
	case dtBYTE:
		if (Field.endianswap()) cdata.swap_endian();
		n = fwrite(cdata.pvoid(), nbytes(), 1, filepointer());		
		break;
	default: printf("ILSegment::read() Unknown type"); return false;
	}

	if (n != 1){
		printf("ILSegment::writebuffer Error writing to file %s\n", Field.datafilepath().c_str());
		return false;
	}
	return true;
}

const size_t& ILField::nlines() const { return Dataset.nlines(); }

const std::string& ILField::datasetpath() const 
{
	return Dataset.datasetpath;
}

bool ILField::initialise_existing(const std::string& fieldname)
{
	Name = fieldname;
	pFile = (FILE*)NULL;
	open();
	parse_datum_projection();
	close();
	return true;
}

bool ILField::create_new(const std::string& fieldname, const IDatatype& _datatype, const size_t& _nbands, const bool& _indexed)
{
	Name  = fieldname;
	pFile = (FILE*)NULL;

	int16_t hdata[256];
	for (size_t i = 0; i < 256; i++) hdata[i] = 0;

	hdata[79] = (int16_t)IHeader::nbytes();
	hdata[81] = 1;

	Header.endianswap = false;
	Header.headeroffset = IHeader::nbytes();
	Header.nlines = Dataset.nlines();
	Header.nbands = _nbands;

	hdata[32] = (int16_t)Dataset.maxspl();
	hdata[33] = (int16_t)Dataset.nlines();
	hdata[34] = (int16_t)_nbands;	

	hdata[217] = (int16_t)Dataset.maxspl();
	hdata[219] = (int16_t)Dataset.nlines();
	hdata[221] = (int16_t)_nbands;

	if (_indexed) {
		Header.accesstype = atINDEXED;
		Header.maxspl = Dataset.maxspl();
		hdata[91] = 2; //INDEXED		
	}
	else {
		Header.accesstype = atDIRECT;
		Header.maxspl = 1;
		hdata[91] = 1; //Group By		
	}

	Header.filetype = ftLINE;
	hdata[72] = 1000; //LINE

	Header.datatype = _datatype;
	if (_datatype.isbyte()) { hdata[73] = 1; hdata[90] = 8; }
	else if (_datatype.isshort()) { hdata[73] = 2; hdata[90] = 16; }
	else if (_datatype.isint()) { hdata[73] = 2; hdata[90] = 32; }
	else if (_datatype.isfloat()) { hdata[73] = 3; hdata[90] = 32; }
	else if (_datatype.isdouble()) { hdata[73] = 3; hdata[90] = 64; }

	hdata[93] = -1; //PROJECTION

	if (_nbands > 1) {
		Header.bandpackingtype = bptBIP;
		hdata[78] = 2; //BIP
	}
	else {
		Header.bandpackingtype = bptBIL;
		hdata[78] = 1; //BIL
	}

	Header.indexname = "INDEX                    ";//INDEX FILE - need spaces
	strncpy((char*)&hdata[87], "OK", 2);  //???
	strncpy((char*)&hdata[89], "P1", 2);  //???
	strncpy((char*)&hdata[171], Header.indexname.c_str(), 25);//INDEX FILE - need spaces
	hdata[185] = 1;
	hdata[186] = 251;

	if ((pFile = fileopen(datafilepath(), "w+b")) == NULL) {
		glog.logmsg("Cannot create file: %s\n\n", datafilepath().c_str());
		return false;
	}

	if (fwrite((char*)hdata, IHeader::nbytes(), 1, pFile) != 1) {
		glog.logmsg("Cannot write header: %s\n\n", datafilepath().c_str());
		return false;
	}

	for (size_t li = 0; li < nlines(); li++) {
		size_t ns = Dataset.nsamplesinline(li);
		std::vector<std::vector<float>> array;
		array.resize(nbands());
		for (size_t bi = 0; bi < nbands(); bi++) {
			array[bi].resize(ns, 0.0);
		}
		//Segments[si].writearray(array);
	}
	close();

	FILE* lfile;
	if ((lfile = fileopen(dotdotlinefilepath(), "w+b")) == NULL) {
		glog.logmsg("Cannot create file: %s\n\n", dotdotlinefilepath().c_str());
		return false;
	}
	fclose(lfile);
	open();
	close();
}

#endif
