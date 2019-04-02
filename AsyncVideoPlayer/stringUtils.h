#ifndef STRING_UTILS_H
#define STRING_UTILS_H


const char* findFileName(char *filepath)
{
	//char * filename;
	char *slash = filepath, *next;
	//iterate through all the backslash or forward slashes in the string
	//to find the last one 
    while ((next = strpbrk(slash + 1, "\\/"))) slash = next;
    //advance beyond the final slash by one index
	if (filepath != slash) slash++;
	//strncpy(*p, (const char*)pf, strlen(pf));//strdup(pf, slash - pf);
    //filename = slash;//strdup(slash);  //duplicate the substring in pf starting at index slash

	return slash;

}

const char* findFileExtension(const char *fileNameOrPath) 
{
    const char *dot = strrchr(fileNameOrPath, '.');
    if(!dot || dot == fileNameOrPath) return "";
    return dot + 1;
}


char * getPathWithDifferentExtension( const char * sourcePath, const char * desiredExtension)
{
	const char * filename = findFileName((char*)sourcePath);
	int sourcePathLength = strlen(sourcePath) - strlen(filename);

	//printf( "\nImport from TDF: %s\n\n", filename);

	const char * fileExtension = findFileExtension(sourcePath);
	int fileExtensionLength = strlen(fileExtension);
	int fileNameLength = strlen(filename);
	int filenamebaseLength = fileNameLength - fileExtensionLength;

	char * filenamebase = (char*)malloc( (filenamebaseLength+1) * sizeof(char)); 
	memcpy(filenamebase, filename, filenamebaseLength);
	filenamebase[filenamebaseLength] = '\0';

	int tdfPathLength = sourcePathLength + filenamebaseLength + 3;
	char * newPath = (char*)malloc( ( tdfPathLength + 1 ) * sizeof(char) );
	memcpy(newPath, sourcePath, sourcePathLength);
	newPath[sourcePathLength] = '\0';
	strncat(newPath, filenamebase, filenamebaseLength);
	strcat(newPath, desiredExtension);

	free(filenamebase);

	return newPath;
}

#endif