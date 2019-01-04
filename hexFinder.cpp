#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <chrono>
#include <sys/stat.h>
#include <string>
using namespace std;
using namespace boost::interprocess;

int oneHexDigit(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else throw "bad hex digit";
}

vector<unsigned char> fromHex(string hexToFind)
{
	vector<unsigned char> v;
	for (int i = 0; i < hexToFind.length(); i+=2)
	{
		char h_char = hexToFind[i];
		char l_char = hexToFind[i+1];
		int b = oneHexDigit(h_char) * 16 + oneHexDigit(l_char);
		v.push_back(b);
	}
	return v;
}

size_t dirsize;
size_t currenSize;
chrono::time_point<chrono::system_clock> start;

void secToString(long long nsec, char *buf)
{
	if (nsec > 3600)
		sprintf(buf, "%.2fhours", nsec / 3600.0);
	else if (nsec > 60)
		sprintf(buf, "%.2fmin", nsec / 60.0);
	else 
		sprintf(buf, "%lld", nsec);
}

bool search(const char *fileNameToSearch, unsigned char* pat, unsigned char* txt, size_t patlen, size_t textlen)
{
	bool found = false;
	for (size_t i = 0; i <= textlen - patlen; i++)
	{
		if ((i & ((2 << 24) - 1)) == 0)
		{
			size_t bytes = currenSize + i;
			printf("\r%.2f%% %.2fGB ", (double)(bytes) / dirsize * 100, (double)(bytes)/1e9);
			auto current = chrono::system_clock::now();
			long long elapsed = chrono::duration_cast<chrono::seconds>(current - start).count();
			double speed;
			if (elapsed > 0)
			{
				speed = bytes / elapsed;			
				char buf[80];
				secToString(elapsed, buf);
				printf("%ss %.2fMB/s ", buf, speed / 1e6);
				double remaining = (long long)((dirsize - bytes) / speed);
				secToString(remaining, buf);
				printf("remain: %s", buf);
			}
		}
		size_t j;
		for (j = 0; j < patlen; j++)
			if (txt[i + j] != pat[j])
				break;
		if (j == patlen)
		{
			printf("\n%s: at %ld\n", fileNameToSearch, i);
			found = true;
		}
	}
	return found;
}

bool searchOneFile(const char *fileNameToSearch, vector<unsigned char> &v)
{
	file_mapping m_file(fileNameToSearch, read_only);
	mapped_region region(m_file, read_only);
	unsigned char *addr = (unsigned char *)region.get_address();
	std::size_t size = region.get_size();
	bool ret = search(fileNameToSearch, v.data(), addr, v.size(), size);
	currenSize += size;
	return ret;
}

vector<string> collectFiles(const string target_path, const string reg_str, size_t& dirsize)
{
	boost::regex my_filter(reg_str);
	vector<string> all_matching_files;
	boost::filesystem::directory_iterator end_itr; // Default ctor yields past-the-end
	dirsize = 0;
	for (boost::filesystem::directory_iterator i(target_path); i != end_itr; ++i)
	{
		// Skip if not a file
		if (!boost::filesystem::is_regular_file(i->status())) continue;
		boost::smatch what;
		// Skip if no match for V2:
		if (!boost::regex_match(i->path().filename().string(), what, my_filter)) continue;
		// File matches, store it
		dirsize += boost::filesystem::file_size(i->path());
		all_matching_files.push_back(i->path().string());
	}
	return all_matching_files;
}

int main(int argc, char * argv[])
{
        printf("must be 4 parameters\n");
	printf("call hexFinder hexBytes path/to/dir regexpname\n");
	printf("where regexpname is in regex format : .*\\.bin - dosts and slash is needed!\n");
        printf("example: hexFinder 1bc42df5 /home/user/dir \".*\"\n");
        for (int i=0; i<argc; i++)
                    printf("argv[%d]=%s\n",i,argv[i]);
	if (argc != 4)
		return 0;
	vector<unsigned char> v = fromHex(argv[1]);
	vector<string> fileNames = collectFiles(argv[2], argv[3], dirsize);
	printf("whole size = %.3f GB\n", dirsize / 1e9);
	currenSize = 0;
	start = std::chrono::system_clock::now();
	for (string fileName: fileNames)
        {
                //printf("%s\n",fileName.c_str());
                try
                {
   		  searchOneFile(fileName.c_str(), v);
                }
                catch(...)
                {
                   printf("can't open file %s\n",fileName.c_str());
                }
        }
	return 0;
}

