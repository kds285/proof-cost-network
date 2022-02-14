#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
using namespace std;

string g_sBlackName = "";
string g_sWhiteName = "";

void readName( string sLine );

int main(int argc, char **argv)
{
	if ( argc != 2 ) {
		cerr << "Usage: " << argv[0] << " filename" << endl;
	} else {
		string filename = argv[1];
		filename = filename + "/" + filename + ".dat";
		ifstream ifs(filename.c_str(), ifstream::in);
		string line;
		int Btotal = 0;
		int Wtotal = 0;
		int BW = 0;
		int WW = 0;
		int BL = 0;
		int WL = 0;
		int draw = 0;
		int unknown = 0;
		while ( getline(ifs, line) ) {
			if ( line.size() < 2 )
				continue;
			if ( line[0] == '#' ) {
				readName(line);
				continue;
			}
			//cerr << line << endl;
			istringstream iss(line);
			string token1, token2;
			iss >> token1 >> token1 >> token2;
			char result1 = token1[0];
			char result2 = token2[0];
			if ( result1 == result2 ) {
				if ( Btotal == Wtotal ) {
					// CGI is B
					switch(result1) {
					case 'B': ++BW; break;
					case 'W': ++BL; break;
					case '0': ++draw; break;
					}
				} else {
					// CGI is W
					switch(result1) {
					case 'B': ++WW; break;
					case 'W': ++WL; break;
					case '0': ++draw; break;
					}
				}
			} else {
				++unknown;
			}
			if ( Btotal == Wtotal )
				++Btotal;
			else
				++Wtotal;
		}

		// only show one-side view
		cout << "=== \"" << g_sBlackName << "\" vs \"" << g_sWhiteName << "\" ===" << endl;
		cout << "as " << g_sBlackName << " viewpoint:" << endl;
		cout << "Total: " << Btotal + Wtotal << " game(s)" << endl;
		cout << left << fixed << setprecision(2);
		cout << "W:" << setw(5) << static_cast<double>(BW+WW+(double)draw/2)*100 / (Btotal+Wtotal) << "%      L:" << setw(5) << static_cast<double>(BL+WL+(double)draw/2)*100 / (Btotal+Wtotal) << "%      UNKNOWN:" << static_cast<double>(unknown)*100 / (Btotal+Wtotal) << '%' << endl;
		cout << "BW:" << setw(5) << static_cast<double>(BW)*100 / (Btotal) << "%     BL:" << setw(5) << static_cast<double>(BL)*100 / (Btotal) << '%' << endl;
		cout << "WW:" << setw(5) << static_cast<double>(WW)*100 / (Wtotal) << "%     WL:" << setw(5) << static_cast<double>(WL)*100 / (Wtotal) << '%' << endl;
		cout << "DRAW:" << static_cast<double>(draw)*100 / (Btotal+Wtotal) << '%' << endl;
	}
	return 0;
}

void readName( string sLine )
{
	int findPos;
	if( (findPos=sLine.find("Black:"))!=string::npos ) { g_sBlackName = sLine.substr(sLine.find("Black:")+7); }
	else if( (findPos=sLine.find("White:"))!=string::npos ) { g_sWhiteName = sLine.substr(sLine.find("White:")+7); }
}