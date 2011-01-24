#include "ArgEnv.h"

NAMESPACE_UPP

///////////////////////////////////////////////////////////////////////////////////////////////
// parses an args line to be useable by spawnxx functions
char **BuildArgs(String const &command, String const &argline)
{
	Array<String> args;
	
	// first arg should be command name
	args.Add(command);
	int buflen = command.GetCount() + 1;

	// skips leading spaces
	char c;
	int pos = 0;
	while ((c = argline[pos]) != 0 && isspace(c))
		pos++;

	// loop reading args and putting to array
	while (c)
	{
		String &s = args.Add();
		buflen++;
		while (c && !isspace(c))
		{
			// reads enquoted strings
			if (c == '"')
			{
				c = argline[++pos];
				while (c && c != '"')
				{
					s << c;
					buflen++;
					c = argline[++pos];
				}
				if (c)
					c = argline[++pos];
			}
			else
			{
				s << c;
				buflen++;
				c = argline[++pos];
			}
		}

		// skips trailing spaces
		while (c && isspace(c))
			c = argline[++pos];
	}
	buflen += (args.GetCount() + 1) * sizeof(char *);

	// here we've got an array of args and the total size (in bytes) of them
	// we allocates a  buffer for arg array
	char **buf = (char **)malloc(buflen);

	// we fill the buffer with arg strings
	char **bufindex = buf;
	char *bufpos = (char *)(buf + args.GetCount() + 1);
	int i = 0;
	while (i < args.GetCount())
	{
		String s = args[i];
		strcpy(bufpos, ~s);
		*bufindex++ = bufpos;
		bufpos += s.GetCount() + 1 ;
		i++;
	}
	*bufindex = 0;

	// returns array of args
	return buf;

} // END _BuildArgs()


///////////////////////////////////////////////////////////////////////////////////////////////
// parses environment map and builds env array
char **BuildEnv(const VectorMap<String, String> &env)
{
	// calculates total environment size
	int envSize = 0;
	for (int i = 0; i < env.GetCount(); i++)
		envSize += env.GetKey(i).GetCount() + env[i].GetCount() + 2 + sizeof(char *);
	envSize+=2;

	// we allocates a  buffer for env array
	char **buf = (char **)malloc(envSize);

	// we fill the buffer with env strings
	char **bufindex = buf;
	char *bufpos = (char *)(buf + env.GetCount() + 1);
	int i = 0;
	while (i < env.GetCount())
	{
		String s = env.GetKey(i) + "=" + env[i];
		strcpy(bufpos, ~s);
		*bufindex++ = bufpos;
		bufpos += s.GetCount() + 1 ;
		i++;
	}
	*bufindex = 0;

	// returns array of args
	return buf;

} // END _BuildEnv()

END_UPP_NAMESPACE
