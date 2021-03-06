void pipeCompile(string* CompileOutput, int FileId, const char* lang){
	FILE *fpipe;
    char command[100];
    if(strcmp(lang, "cpp")==0)
    	sprintf(command, "g++ -w -static %s%d/main.cpp -o %s%d/main 2>&1", FILEPATH, FileId, FILEPATH, FileId);
    else if(strcmp(lang, "c") == 0)
    	sprintf(command, "g++ -w -static %s%d/main.c -o %s%d/main 2>&1", FILEPATH, FileId, FILEPATH, FileId);
    else if(strcmp(lang, "java")==0)
		sprintf(command, "javac -nowarn -deprecation %s%d/main.java  2>&1", FILEPATH, FileId);    	
    //ToLogs(command);
    ToLogs("Compiling file...");
	char line[256];
	
	if ( !(fpipe = (FILE*)popen(command,"r")) ){  
	// If fpipe is NULL
		perror("Problems with pipe");
		ToLogs("Problems with pipe");
	}
	else{
		while ( fgets( line, sizeof line, fpipe)){
			CompileOutput -> append(line, strlen(line));
		}
	}
	pclose(fpipe);
}
int pipeNoOfInputFiles(const char* ProblemId){
    FILE *fpipe;
    char command[100], line[256];
    sprintf(command, "ls %s%s/Input/ -l | egrep -c '^-'", TESTCASESPATH, ProblemId);
	
	if ( !(fpipe = (FILE*)popen(command,"r")) ){  
	// If fpipe is NULL
		perror("Problems with pipe");
		ToLogs("Problems with pipe");
		return -1;
	}
	else{
		while ( fgets( line, sizeof line, fpipe)){
			return atoi(line);
		}
	}
	pclose(fpipe);
}
void pipeMatch(bool* result, char const* command){
    FILE *fpipe;
    char line[256];
    
	if ( !(fpipe = (FILE*)popen(command,"r")) ){  
	// If fpipe is NULL
		perror("Problems with pipe");
		ToLogs("Problems with pipe");
	}
	else{
		while ( fgets( line, sizeof line, fpipe)){
			char tmp[90];
			sprintf(tmp, "%s", line);
			ToLogs(tmp);
			(*result) = false;
		}
	}
	pclose(fpipe);
}


class FileHandle{
public:
	int FileId, MemoryUsed, NoOfInputFiles, TestCaseId;
	float TimeUsed;
	char systemString[100], status[10], logs[100], detailStatus[10000], str[100], *token, tmp[10], logString[100];
	char timeused[10], memoryused[10], fileid[10];
	const char* lang, *ProblemId;
	bool result;
	string CompileOutput;
	CurlWrapper FileCurl;
	
	FileHandle(int fid, const char* pid, const char* l){
		FileId = fid;
		TimeUsed=0.0;
		MemoryUsed=0;
		ProblemId = pid;
		lang = l;
		result=true;
		sprintf(detailStatus,"\0");
		
	}
	
	void SendResults(){
		sprintf(timeused, "%0.3f", TimeUsed);
		sprintf(memoryused, "%d", MemoryUsed);
		sprintf(fileid, "%d", FileId);
		//sprintf(logs, "%d %s %s %s %s", FileId, status, detailStatus, timeused, memoryused); ToLogs(logs);
		sprintf(logs, "%s %s %s %s", fileid, status, timeused, memoryused); ToLogs(logs);
		printf("%s\n",detailStatus);
		FileCurl.SendResultsToWebpage(fileid, status, detailStatus, timeused, memoryused);
	}
	
	int FetchFile(){
		if(FTPON) {
			int res = FileCurl.GetFileFromFTP(FileId);
			if(res==-1)	return -1;
		}
		else if(HTTPON){
			int res = FileCurl.GetFileFromHTTP(FileId);
			if(res==-1)	return -1;
		}
		return 0;
	}
	
	int MakeDir(){
		int ErrNo;
		char dirString[100];
		sprintf(dirString, "%s%d",FILEPATH, FileId);
		ToLogs("Creating directory with File Id");
		if( mkdir(dirString, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_IWOTH)==-1){
			ErrNo=errno;
			if(ErrNo==17) ToLogs("Directory already created.. Continuing");
			else {
			sprintf(dirString, "IE ERROR Failure to create directory. Error Number: %d", errno);
				ToLogs(dirString);
				return -1;
			}
		}
		sprintf(systemString, "cp %s%d.txt %s%d/main.%s", FILEPATH, FileId, FILEPATH, FileId, lang);
		ToLogs(systemString);	system(systemString);
		return 0;
	}
	
	void Compile(){
		pipeCompile(&CompileOutput, FileId, lang);
		if(CompileOutput.length()!=0){
			ToLogs("Compilation unsuccessful"); //ToLogs(CompileOutput.c_str());
			result = false;
			strcpy(status, "CE");
			strcpy(detailStatus, CompileOutput.c_str());
		}
		else ToLogs("Compilation successful\n");
	}
	
	int PrepareToExecute(){
		sprintf(systemString, "cp %s%s/Input/* %s%d/", TESTCASESPATH, ProblemId, FILEPATH, FileId);
		system(systemString);
		
		NoOfInputFiles = pipeNoOfInputFiles(ProblemId);
		if(NoOfInputFiles==0){
			ToLogs("IE ERROR No Input File Specified. Please rectify the problem\n");
			return -1;
		}
		sprintf(systemString, "Number of Test Cases = %d",NoOfInputFiles);
		ToLogs(systemString);
		return 0;
	}
	
	void PipeExecute( int TimeLimit, int MemoryLimit, string* ExecutionStr){
		FILE *fpipe;
	    char command[100];
	    if(strcmp(lang,"cpp")==0 || strcmp(lang,"c")==0){
	    	sprintf(command, "./Execution %d %d %d %d %s", FileId, TestCaseId, TimeLimit, MemoryLimit, lang);
	    }
	    else if(strcmp(lang, "java")==0){
	    	sprintf(command, "./java_Execution %d %d %d %d %s", FileId, TestCaseId, TimeLimit, MemoryLimit, lang);
	    }
    	ToLogs(command);
		char line[1024];
	
		if ( !(fpipe = (FILE*)popen(command,"r")) ){  
		// If fpipe is NULL
			perror("Problems with pipe");
			ToLogs("Problems with pipe");
		}
		else{
			while ( fgets( line, sizeof line, fpipe)){
				ExecutionStr -> append(line, strlen(line));
			}
		}
		pclose(fpipe);
	}

	
	void Execution(int TimeLimit, int MemoryLimit){
		
		for(TestCaseId=1; TestCaseId<=NoOfInputFiles; TestCaseId++){
			string ExecutionStr;
			PipeExecute(TimeLimit, MemoryLimit, &ExecutionStr);
			strcpy(str, ExecutionStr.c_str()); ToLogs(str);
			token = strtok(str, " \n");
			strcpy(status, token);
			if(strcmp(token, "AC")!=0) result=false;
			if(strcmp(token, "RE")==0 || strcmp(token, "IE")==0 ){
				token = strtok(NULL, "\n");
				strcpy(detailStatus, token);
			}
			token = strtok(NULL, " \n"); sprintf(tmp, "%s", token);
			TimeUsed+=(float)atof(tmp);
			if(TimeUsed>(float)TimeLimit){
				result = false;
				sprintf(status, "TLE");
				sprintf(detailStatus, "\0");
			}
			if(result==false) break;
		}
		
		for(TestCaseId=1; (result==true && TestCaseId<=NoOfInputFiles); TestCaseId++){
			MatchOutput();
			if(result==false) strcpy(status,"WA");
		}
	}
	
	void MatchOutput(){
		char FromFileStr[100], ToFileStr[100];
		sprintf(FromFileStr, "%s%s/Output/%d.txt", TESTCASESPATH, ProblemId, TestCaseId);
		sprintf(ToFileStr, "%s%d/%do.txt", FILEPATH, FileId, TestCaseId);
		char cmd[100];
		sprintf(cmd, "diff %s %s --ignore-all-space --ignore-blank-lines --ignore-tab-expansion --ignore-space-change --brief 2>&1", FromFileStr, ToFileStr);
		pipeMatch(&result, cmd);
	}
	
	void CleanUp(){
		sprintf(systemString, "rm -rf %s%d", FILEPATH, FileId);
		system(systemString);
	}

};
