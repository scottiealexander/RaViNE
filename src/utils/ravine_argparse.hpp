#ifndef RAVINE_ARGPARSE_HPP_
#define RAVINE_ARGPARSE_HPP_

#include <string>
#include <cstdio>
#include <cstdlib>

namespace RVN
{
    /* ---------------------------------------------------------------------- */
    int arg_parse(const char** args, int narg,
        std::string& dev, std::string& rffile, std::string& ofile, int& port,
        bool& save, bool& listen)
    {
        dev = "/dev/video0";
        rffile = "./rf/rf-05.pgm";
        ofile = "";
        port = -1;
        save = false;
        listen = false;

        int k = 1;
        while (k < narg)
        {
            std::string tmp(args[k]);

            if (tmp == "-h")
            {
                return -1;
            }
            else if (tmp == "-p")
            {
                if (narg > (k + 1))
                {
                    port = std::atoi(args[k+1]);
                    listen = true;
                    k += 2;
                }
            }
            else if (tmp == "-f")
            {
                if (narg > (k + 1))
                {
                    ofile.assign(args[k+1]);
                    save = true;
                    k += 2;
                }
            }
            else if (tmp == "-r")
            {
                if (narg > (k + 1))
                {
                    rffile.assign(args[k+1]);
                    k += 2;
                }
            }
            else if (tmp == "-d")
            {
                if (narg > (k + 1))
                {
                    dev.assign(args[k+1]);
                    k += 2;
                }
            }
            else
            {
                printf("[ERROR]: invalid input \"%s\"\n", tmp.c_str());
                return -1;
            }
        }

        printf("Port: %d | save: %d | listen: %d | ofile: %s | rffile: %s\n",
            port, save, listen, ofile.c_str(), rffile.c_str());

        if (listen && (port < 1 || port > 65535))
        {
            printf("[ERROR]: invalid port %d\n", port);
            return -1;
        }

        if (save && ofile.empty())
        {
            printf("[ERROR]: cannot set save to true with out valid output file (-f)\n");
            return -1;
        }
        return 0;
    }
    /* ---------------------------------------------------------------------- */
}

#endif
