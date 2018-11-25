enum
{
	ELI_UNRESOLVED,		// don't know what type this is
	ELI_INTEGER,		// it's an integer
	ELI_STRING,			// it's a string
};

struct EXPRESSION_LIST_ITEM
{
	unsigned int
		itemType;		// tells what type of value we've got
	bool
		valueResolved;	// tells if the value is known or not
	int
		itemValue;
	char
		itemString[MAX_STRING];	// item string
};


bool ParseTypedExpression(const char *line,unsigned int *lineIndex,EXPRESSION_LIST_ITEM *expressionListItem);
bool ParseExpression(const char *line,unsigned int *lineIndex,int *value,bool *unresolved);
