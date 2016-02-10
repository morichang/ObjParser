// ObjParse.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <boost/fusion/adapted/struct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix.hpp>
#include <fstream>
#include <vector>

struct float3d{
	float x, y, z;
};

struct float2d{
	float x, y;
};

typedef unsigned long u32;

BOOST_FUSION_ADAPT_STRUCT(
	float3d,
	(float, x)
	(float, y)
	(float, z)
);

BOOST_FUSION_ADAPT_STRUCT(
	float2d,
	(float, x)
	(float, y)
);

class ObjFile {
public:
	class Group {
	public:
		std::string name;
		std::string material;
		std::vector<float3d> vertexList;
		std::vector<float3d> normalList;
		std::vector<float2d> texCoordList;
	};

	class Index {
	public:
		u32 vertex, texcoord, normal;
	};

	class Face {
	public:
		Index i1, i2, i3;
	};

	std::string materialFile;
	std::vector<Group> groups;
};

BOOST_FUSION_ADAPT_STRUCT(
	ObjFile,
	(std::string, materialFile)
	(std::vector<ObjFile::Group>, groups)
);

BOOST_FUSION_ADAPT_STRUCT(
	ObjFile::Group,
	(std::string, name)
	(std::string, material)
	(std::vector<float3d>, vertexList)
	(std::vector<float3d>, normalList)
	(std::vector<float2d>, texCoordList)
	(std::vector<ObjFile::Face>, faceList)
);

BOOST_FUSION_ADAPT_STRUCT(
	ObjFile::Index,
	(u32, vertex)
	(u32, texcoord)
	(u32, normal)
);

BOOST_FUSION_ADAPT_STRUCT(
	ObjFile::Face,
	(ObjFile::Index, i1)
	(ObjFile::Index, i2)
	(ObjFile::Index, i3)
);

using namespace boost::spirit;

template <typename Iterator>
class ObjParser : public qi::grammar<Iterator, ObjFile()>
{
	qi::rule<Iterator, float3d()> float3d_;
	qi::rule<Iterator, float2d()> float2d_;
	qi::rule<Iterator, ObjFile::Index()> index;

	qi::rule<Iterator, float3d()> vert_line;
	qi::rule<Iterator, float3d()> normal_line;
	qi::rule<Iterator, float2d()> texcoord_line;
	qi::rule<Iterator, std::string()> group_line;
	qi::rule<Iterator, std::string()> usemtl_line;
	qi::rule<Iterator, std::string()> mtllib_line;
	qi::rule<Iterator, ObjFile::Face()> face_line;
	qi::rule<Iterator> surface_line;
	qi::rule<Iterator> comment_line;

	qi::rule<Iterator, ObjFile::Group()> group;

	qi::rule<Iterator, ObjFile()> start;

public:
	ObjParser()
		: ObjParser::base_type(start)
	{
		using namespace qi;
		namespace phx = boost::phoenix;

		float3d_ %= float_ >> ' ' >> float_ >> ' ' >> float_;
		float2d_ %= float_ >> ' ' >> float_;
		index = eps[_val = phx::construct<ObjFile::Index>()]
			>> uint_[phx::at_c<0>(_val) = qi::_1]
			>> !('/' >> uint_[phx::at_c<1>(_val) = qi::_1]
			>> !('/' >> uint_[phx::at_c<2>(_val) = qi::_1])
			);

		vert_line %= "v " >> float3d_ >> eol;
		normal_line %= "vn " >> float3d_ >> eol;
		texcoord_line %= "vt " >> float2d_ >> eol;

		group_line %= "g " >> +(char_ - eol) >> eol;

		face_line %= "f " >> index >> ' ' >> index >> ' ' >> index >> eol;
		surface_line = "s " >> *(char_ - eol) >> eol;

		usemtl_line %= "usemtl " >> +(char_ - eol) >> eol;
		mtllib_line %= "mtllib " >> +(char_ - eol) >> eol;

		comment_line = '#' >> *(char_ - eol) >> eol | eol;

		group = eps[_val = phx::construct<ObjFile::Group>()]
			>> group_line[phx::at_c<0>(_val) = qi::_1]
			>> *(
			usemtl_line[phx::at_c<1>(_val) = qi::_1]
			| vert_line[phx::push_back(phx::at_c<2>(_val), qi::_1)]
			| normal_line[phx::push_back(phx::at_c<3>(_val), qi::_1)]
			| texcoord_line[phx::push_back(phx::at_c<4>(_val), qi::_1)]
			| comment_line
			);

		start = eps[_val = phx::construct<ObjFile>()]
			>> *(
			group[phx::push_back(phx::at_c<1>(_val), qi::_1)]
			| mtllib_line[phx::at_c<0>(_val) = qi::_1]
			| comment_line
			);
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc == 0) {
		std::cout << "require one argument : .obj filepath" << std::endl;
		return -1;
	}

	std::ifstream file("merged_mesh.obj");
	std::string source;
	for (char c; !file.eof() && file.get(c);) {
		source.push_back(c);
	}
	file.close();

	ObjFile value;
	ObjParser<std::string::iterator> parser;
	qi::parse(source.begin(), source.end(), parser, value);

	return 0;
}

