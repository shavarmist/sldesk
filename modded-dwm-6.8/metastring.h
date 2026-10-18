
#include <stddef.h>

typedef struct _MetaStringProperty {
	char* key;
	char* value;
	struct _MetaStringProperty* next;
} MetaStringProperty;

typedef struct {
	char* content;
	MetaStringProperty* properties;

	size_t bump_ptr;
	size_t arena_siz;
	char arena[];
} MetaString;

MetaString* parse_metastring(const char*);

