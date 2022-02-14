#pragma once

#include <cstdio>
#include <string>
#include <ctime>
#include <boost/date_time/posix_time/posix_time.hpp>

class TimeSystem {
public:
	/*!
	@brief  get the current time
	@author kwchen
	*/
	static boost::posix_time::ptime getLocalTime() { return boost::posix_time::microsec_clock::local_time(); }

	/*!
	@brief  get format string
	@author T.F. Liao
	@author kwchen
	@param  sFormat [in]    the format of output string
	@param  timeStamp   [in]    the timestamp to be translated
	@return the formated string
	*/
	static std::string getTimeString(std::string sFormat = "Y/m/d H:i:s", boost::posix_time::ptime localtime = getLocalTime()) {
		std::string sResult;
		bool bEscape = false;

		for (size_t i = 0; i < sFormat.length(); ++i) {
			if (bEscape) {
				sResult += sFormat.at(i);
				bEscape = false;
			} else {
				switch (sFormat.at(i)) {
				case 'Y': sResult += translateIntToString(localtime.date().year()); break;
				case 'y': sResult += translateIntToString(localtime.date().year() % 100, 2); break;
				case 'm': sResult += translateIntToString(localtime.date().month(), 2); break;
				case 'd': sResult += translateIntToString(localtime.date().day(), 2); break;
				case 'H': sResult += translateIntToString(localtime.time_of_day().hours(), 2); break;
				case 'i': sResult += translateIntToString(localtime.time_of_day().minutes(), 2); break;
				case 's': sResult += translateIntToString(localtime.time_of_day().seconds(), 2); break;
				case 'f': sResult += translateIntToString(localtime.time_of_day().total_milliseconds() % 1000, 3); break;
				case 'u': sResult += translateIntToString(localtime.time_of_day().total_microseconds() % 1000000, 6); break;
				case '\\': bEscape = true; break;
				default: sResult += sFormat.at(i); break;
				}
			}
		}
		return sResult;
	}

private:
	/*!
	@breif  translate integer to string with specified minimal width of output
	@author T.F. Liao
	@param  iVal    [in] integer to be translate
	@param  iWidth  [in] minimal width
	@return the translated string
	*/
	static std::string translateIntToString(int iVal, int iWidth = 0) {
		char buf[16];
		static char zeroFillFormat[] = "%0*d", nonZeroFillFormat[] = "%*d";

		if (iWidth > 15) iWidth = 15;
		if (iWidth < 0) iWidth = 0;

		char *format = (iWidth ? zeroFillFormat : nonZeroFillFormat);
		sprintf(buf, format, iWidth, iVal);
		return buf;
	}
};