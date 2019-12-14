#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <vector>
#include <dirent.h>
#include <string>
#include <queue>
#include <iostream>
#include <thread>
using namespace std;

namespace filelist
{
    queue <string> filequeue;
    //char initdirpath[MAXDIRPATH];
    string initdirpath;
    bool onlycur;
    bool listmade;
    char *pat;
}

//Preliminary declarations
int godir(string dirpath);
void computeLPSArray(char* pat, unsigned int patlen, unsigned int* lps);

// Prints occurrences of txt[] in pat[]
void KMPSearch(char* pat, char* txt, char* file, unsigned int txtlen)
{
    unsigned int patlen = strlen(pat);
    unsigned int lastprinted = 0;
    // create lps[] that will hold the longest prefix suffix
    // values for pattern
    unsigned int lps[patlen];

    // Preprocess the pattern (calculate lps[] array)
    computeLPSArray(pat, patlen, lps);

    unsigned int i = 0; // index for txt[]
    unsigned int j = 0; // index for pat[]
		unsigned int strnum = 1;
    while (i < txtlen)
    {
				if (txt[i] == '\n')
					strnum++;
        if (pat[j] == txt[i])
        {
            j++;
            i++;
        }
        if (j == patlen)
        {
            unsigned int cur = i - j;
						while ((cur > 0) && (txt[cur] != '\n'))
							cur--;
						cur++;
						unsigned int strbegin, strend;
						strbegin = cur;
						cur = i - j;
						//vector <char> output;
						while ((cur < txtlen - 1) && (txt[cur] != '\n'))
							cur++;
						strend = cur - 1;
            if (strnum != lastprinted)
              printf("%s : %d : %.*s\n", file, strnum, strend - strbegin + 1, txt + strbegin);
            lastprinted = strnum;
        		j = lps[j - 1];
        }
        // mismatch after j matches
        else if (i < txtlen && pat[j] != txt[i])
        {
            // Do not match lps[0..lps[j-1]] characters,
            // they will match anyway
            if (j != 0)
                j = lps[j - 1];
            else
                i = i + 1;
        }

    }
}

// Fills lps[] for given patttern pat[0..M-1]
void computeLPSArray(char* pat, unsigned int M, unsigned int* lps)
{
    // length of the previous longest prefix suffix
    int len = 0;

    lps[0] = 0; // lps[0] is always 0

    // the loop calculates lps[i] for i = 1 to M-1
    unsigned int i = 1;
    while (i < M) {
        if (pat[i] == pat[len]) {
            len++;
            lps[i] = len;
            i++;
        }
        else // (pat[i] != pat[len])
        {
            // This is tricky. Consider the example.
            // AAACAAAA and i = 7. The idea is similar
            // to search step.
            if (len != 0) {
                len = lps[len - 1];

                // Also, note that we do not increment
                // i here
            }
            else // if (len == 0)
            {
                lps[i] = 0;
                i++;
            }
        }
    }
}

void makefilelist()
{
  godir(filelist::initdirpath);
  filelist::listmade = true;
}


int searchinfile()
{
  while (filelist::filequeue.empty())
  {
    if (filelist::listmade)
      return 0;
  }
  while (not filelist::filequeue.empty())
  {
    string strcurfile = filelist::filequeue.front();
    filelist::filequeue.pop();
    char *curfile = new char[strcurfile.length() + 1];
    curfile = strcpy(curfile, strcurfile.c_str());
    int fd = open(curfile, O_RDWR, 0600);
    if (fd < 0)
    {
      perror(curfile);
      delete[] curfile;
      close(fd);
      continue;
    }
    unsigned long long fsize = 0;
    char *buf = new char[1];
    lseek(fd, 0, SEEK_SET);
    while (1 == read(fd, buf, 1))
    {
      fsize++;
      lseek(fd, 1, SEEK_CUR);
    }
    void *memstart = mmap(NULL, fsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    KMPSearch(filelist::pat, (char *) memstart, curfile, fsize);

    munmap(memstart, fsize);
    close(fd);
    delete[] curfile;
    delete[] buf;
  }
  return 0;
}

int godir(string dirpath)
{
	DIR *dir = opendir(&dirpath[0]);
	if (dir == NULL)
	{
		perror(&dirpath[0]);
		return -1;
	}
	struct dirent *rd = readdir(dir);
	while (rd != NULL)
	{
		while (((rd->d_type != DT_REG) && (rd->d_type != DT_DIR)) || (rd->d_type == DT_FIFO))
			rd = readdir(dir);
		if (rd->d_type == DT_REG)
      filelist::filequeue.push(string(dirpath) + "/" + string(rd->d_name));
		else if ((not filelist::onlycur) && (rd->d_type == DT_DIR))
		{
      if (strcmp(rd->d_name, ".") == 0 || strcmp(rd->d_name, "..") == 0)
			{
				rd = readdir(dir);
				continue;
			}
      string newdirpath = (string) dirpath + "/" + (string) rd->d_name;
      godir(newdirpath);
			}
		rd = readdir(dir);
}
  free(rd);
	closedir(dir);
	return 0;
}


int main(int argc, char** argv)
{
	int patlen = -1;
	int numstreams = 1;
	filelist::onlycur = 0;
  filelist::listmade = false;
  filelist::initdirpath = ".";
  //Чтение аргументов
  {
	if (argc == 1)
	{
		printf("Enter the pattern to search for\n");
		return 0;
	}
	else if (argc > 1)
	{
		int i = 1;
		while (i < argc)
		{
			char c = argv[i][0];
			if (c == '-')
			{
				c = argv[i][1];
				if (c == 'n')
					filelist::onlycur = 1;
				else if (c == 't')
				{
					string cnumstreams = "";
					int j = 2;
					while (argv[i][j] != '\0')
					{
						cnumstreams = cnumstreams + argv[i][j];
						j++;
					}
					if (numstreams > 1)
					{
						printf("There cannot be two or more -t options\n");
						return 0;
					}
          numstreams = stoi(cnumstreams);
          if (numstreams < 1)
          {
            printf("Enter a valid number of streams\n");
            return 0;
          }
				}
			}
			else if (c == '/')
			{
				int j = 0;
				//предполагаем, что путь начинается не со слеша
				//dirpath[0] = '\';
				if (filelist::initdirpath[0] != '.')
				{
					printf("You cannot specify two or more folders\n");
					return 0;
				}
				while (argv[i][j] != '\0')
				{
					filelist::initdirpath = filelist::initdirpath +argv[i][j];
					j++;
				}
			}
			else
			{
				patlen = 0;
				while (argv[i][patlen] != '\0')
					patlen++;
				filelist::pat = new char[patlen + 1];
				int j = 0;
				for (; j < patlen; j++)
					filelist::pat[j] = argv[i][j];
				filelist::pat[j] = '\0';
			}
			i++;
		}
	}
}
  //Завершен блок чтения аргументов

  //Запуск нужного количества потоков

  std::thread thrgo[numstreams];
  std::thread thrmake(makefilelist);

  if (numstreams > 1)
  {
    for (int i = 0; i < numstreams - 1; i++)
    {
      thrgo[i] = std::thread(searchinfile);
    }
  }
  thrmake.join();
  searchinfile();

  for (int i = 0; i < numstreams - 1; i++)
  {
      thrgo[i].join();
  }
  delete[] filelist::pat;
}
