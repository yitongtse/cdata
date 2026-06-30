#include <iostream.h>

main()
{
	char line[1024];
	while (cin) {
		cin.getline(line, 1024);
		cin.getline(line, 1024);
		cin.getline(line, 1024);
		cin.getline(line, 1024);
		cout << line << endl;
	}
	return 0;
}
