#ifndef WAVEFRONT_OBJECTS_ADAPTER_H
#define WAVEFRONT_OBJECTS_ADAPTER_H
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <functional>

class OBJAdapter
{
public:
	enum IOOptions
	{
		VertexNormals = 0x1,
		VertexTexCoords = 0x2,
		None,
		UnKnown
	};
	struct Face
	{
		std::vector<unsigned> faceIdx;
		std::vector<unsigned> texcoordIdx;
		std::vector<unsigned> vertexNormalIdx;
	};
	struct Point
	{
		double data[3];
		double & operator[](unsigned i)
		{
			return data[i];
		}
	};
	struct TexCoord
	{
		double u, v;
	};
public:
	OBJAdapter(std::istream & in)
	{
		m_IOOptions = IOOptions::UnKnown;
		std::string line;
		unsigned lineNum = 0;
		while (std::getline(in, line)) {
			lineNum++;
			line = trim_line(line);
				
			if (line.empty() || is_comment_line(line))
				continue;
			else if (is_vertex_line(line)) {
				if (parse_vertex_line(line) == false) {
					std::cerr << "Invalid vertices data: #line " << lineNum << std::endl;
					clear();
					return;
				}
			}
			else if (is_face_line(line)) {
				if (parse_face_line(line) == false) {
					std::cerr << "Invalid face data: #line " << lineNum << std::endl;
					clear();
					return;
				}
			}
			else if (is_vertex_normal_line(line)) {
				if (!parese_vertex_normal_line(line)) {
					std::cerr << "Invalid vertex normal data: #line " << lineNum << std::endl;
					clear();
					return;
				}
			}
			else if (is_vertex_texcoord_line(line)) {
				if (!parse_vertex_texcoord_line(line)) {
					std::cerr << "Invalid vertex texcoord data: #line " << lineNum << std::endl;
					clear();
					return;
				}
			}
			else {
				//other lines are ignored
			}
		}
	}
	void clear()
	{
		m_IOOptions = IOOptions::UnKnown;
		m_vertices.clear();
		m_faces.clear();
		m_vertexNormals.clear();
		m_texcoord.clear();
	}

	bool has_vertex_normals()const
	{
		return (m_IOOptions & IOOptions::VertexNormals) != 0;
	}

	bool has_vertex_texcoord()const
	{
		return (m_IOOptions & IOOptions::VertexTexCoords) != 0;
	}
	bool is_valid()const
	{
		return m_IOOptions != IOOptions::UnKnown;
	}
	void write(std::ostream &o)
	{
		if (is_valid() == false)return;

		for (auto &v : m_vertices) {
			o << "v " << v[0] << ' ' << v[1] << ' ' << v[2] << std::endl;
		}
		if (has_vertex_normals()) {
			for (auto &v: m_vertexNormals )
				o << "vn " << v[0] << ' ' << v[1] << ' ' << v[2] << std::endl;
		}
		if (has_vertex_texcoord()) {
			for (auto &uv : m_texcoord) {
				o << "vt " << uv.u << ' ' << uv.v << std::endl;
			}
		}

		if (has_vertex_normals() == false && has_vertex_texcoord() == false) {
			for (auto & f : m_faces) {
				o << "f";
				for (auto &v : f.faceIdx)
					o << ' ' << v;
				o << std::endl;
			}
		}
		else if (has_vertex_normals() && has_vertex_texcoord()) {
			for (auto & f : m_faces) {
				o << "f";
				for (unsigned i = 0; i < f.faceIdx.size(); ++i) {
					o << ' ' << f.faceIdx[i] << '/' << f.texcoordIdx[i] << '/' << f.vertexNormalIdx[i];
				}
				o << std::endl;
			}
		}
		else if (has_vertex_normals()) {
			for (auto & f : m_faces) {
				o << "f";
				for (unsigned i = 0; i < f.faceIdx.size(); ++i) {
					o << ' ' << f.faceIdx[i] << '/' <<  '/' << f.vertexNormalIdx[i];
				}
				o << std::endl;
			}
		}
		else {
			for (auto & f : m_faces) {
				o << "f";
				for (unsigned i = 0; i < f.faceIdx.size(); ++i) {
					o << ' ' << f.faceIdx[i] << '/' << f.texcoordIdx[i];
				}
				o << std::endl;
			}
		}
	}
private:
	bool parse_vertex_texcoord_line(std::string &line)
	{
		TexCoord t;
		std::istringstream istr(line.substr(2));
		istr >> t.u >> t.v;
		if (!istr)
			return false;
		m_texcoord.push_back(t);
		return true;
	}

	bool parese_vertex_normal_line(const std::string & line)
	{
		Point n;
		std::istringstream istr(line.substr(2));
		istr >> n[0] >> n[1] >> n[2];
		if (!istr)
			return false;
		m_vertexNormals.push_back(n);
		return true;
	}

	bool parse_vertex_line(const std::string &line) {
		Point pt;
		std::istringstream istr(line.substr(1));
		istr >> pt[0] >> pt[1] >> pt[2];
		if (!istr) {
			return false;
		}
		m_vertices.push_back(pt);
		return true;
	}

	static int determine_face_type(const std::string & line)
	{
		std::string::size_type pos = line.find_first_not_of(' ', 1);
		auto pos2 = line.find_first_of(' ', pos);
		if (std::string::npos == pos || std::string::npos == pos2)
			return -1;
		auto str = line.substr(pos, pos2 - pos);
		pos = str.find('/');
		if (pos == std::string::npos) //f v v v
			return 0;
		pos2 = str.find('/', pos + 1);
		if (std::string::npos == pos2) //f v/vt v/vt v/vt
			return 1;
		if (pos + 1 == pos2) //f v//vn v//vn v//vn
			return 2;
		return 3;
	}

	void setIOOptionsByType(int type)
	{
		switch (type)
		{
		case 0:
			m_IOOptions = IOOptions::None;
			break;
		case 1:
			m_IOOptions = IOOptions::VertexTexCoords;
			break;
		case 2:
			m_IOOptions = IOOptions::VertexNormals;
			break;
		case 3:
			m_IOOptions = IOOptions::VertexNormals | IOOptions::VertexTexCoords;
			break;
		default:
			m_IOOptions = IOOptions::UnKnown;
		}
	}
	bool isIOOptionsMatchType(int type)const
	{
		switch (type)
		{
		case 0:
			return m_IOOptions == IOOptions::None;
		case 1:
			return m_IOOptions == IOOptions::VertexTexCoords;
		case 2:
			return m_IOOptions == IOOptions::VertexNormals;
		case 3:
			return m_IOOptions ==(IOOptions::VertexNormals | IOOptions::VertexTexCoords);
		default:
			return false;
		}
	}

	bool parse_face_line(const std::string & line)
	{
		/* 
		* type value and corresponding face format
		* 0: f v v v
		* 1£ºf v/vt v/vt v/vt
		* 2: f v//vn v//vn v//vn
		* 3: f v/vt/vn v/vt/vn v/vt/vn
		*/
		int type = determine_face_type(line);
		if (-1 == type)
			return false;
		if (IOOptions::UnKnown != m_IOOptions) {
			if (isIOOptionsMatchType(type) == false)
				return false;
		}
		else {
			setIOOptionsByType(type);
		}

		if (0 == type) {
			std::istringstream istr(line.substr(1));
			Face f;
			std::vector<unsigned> & fv = f.faceIdx;
			unsigned t;
			istr >> t;
			while (istr) {
				fv.push_back(t);
				istr >> t;
			}
			if (fv.size() < 3)
				return false;
			m_faces.push_back(std::move(f));
		}
		else if (1 == type) {
			std::istringstream istr(line.substr(1));
			Face f;
			std::vector<unsigned> & fv = f.faceIdx;
			std::vector<unsigned> & ft = f.texcoordIdx;
			unsigned t;
			char c;
			istr >> t;
			while (istr) {
				c = 0;
				istr >> c;
				if ('/' != c)
					return false;
				fv.push_back(t);
				istr >> t;
				if (!istr)
					return false;
				ft.push_back(t);
				istr >> t;
			}
			if (fv.size() < 3)
				return false;
			m_faces.push_back(std::move(f));
		}
		else if (2 == type) {
			std::istringstream istr(line.substr(1));
			Face f;
			std::vector<unsigned> & fv = f.faceIdx;
			std::vector<unsigned> & fn = f.vertexNormalIdx;
			unsigned t;
			char c;
			istr >> t;
			while (istr) {
				c = 0;
				istr >> c;
				if ('/' != c) return false;
				c = 0;
				istr >> c;
				if ('/' != c)return false;
				fv.push_back(t);
				istr >> t;
				if (!istr) return false;
				fn.push_back(t);
				istr >> t;
			}
			if (fv.size() < 3)
				return false;
			m_faces.push_back(std::move(f));
		}
		else {
			std::istringstream istr(line.substr(1));
			Face f;
			std::vector<unsigned> & fv = f.faceIdx;
			std::vector<unsigned> & ft = f.texcoordIdx;
			std::vector<unsigned> & fn = f.vertexNormalIdx;
			unsigned t;
			char c;
			istr >> t;
			while (istr) {
				c = 0;
				istr >> c;
				if ('/' != c)return false;
				fv.push_back(t);
				istr >> t;
				if (!istr)return false;
				c = 0;
				istr >> c;
				if ('/' != c) return false;
				ft.push_back(t);
				istr >> t;
				if (!istr) return false;
				fn.push_back(t);

				istr >> t;
			}			
			if (fv.size() < 3)
				return false;
			m_faces.push_back(std::move(f));
		}
		return true;
	}
private:
	static std::string trim_line(const std::string &line)
	{
		unsigned n = 0;
		while (n < line.length()) {
			if (line[n] == ' ' || line[n] == '\t')
				++n;
			else
				break;
		}
		return line.substr(n);
	}
	static bool is_comment_line(const std::string &line) 
	{
		return line[0] == '#';
	}
	static bool is_vertex_line(const std::string & line)
	{
		if (line.length() >= 2) {
			return line[0] == 'v' && line[1] == ' ';
		}
		return false;
	}
	static bool is_face_line(const std::string & line)
	{
		if (line.length() >= 2) {
			return line[0] == 'f' && line[1] == ' ';
		}
		return false;
	}
	static bool is_vertex_normal_line(const std::string &line)
	{
		if (line.length() >= 3) {
			return line[0] == 'v' && line[1] == 'n' && line[2] == ' ';
		}
		return false;
	}
	static bool is_vertex_texcoord_line(const std::string & line)
	{
		if (line.length() >= 3) {
			return line[0] == 'v' && line[1] == 't' && line[2] == ' ';
		}
		return false;
	}
public:
	std::vector<Point> m_vertices;
	std::vector<Face> m_faces;
	std::vector<Point> m_vertexNormals;
	std::vector<TexCoord> m_texcoord;
	int m_IOOptions;
};

#endif