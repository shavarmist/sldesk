
#include "metastring.h"
#include <stdlib.h>
#include <string.h>
#include "util.h"

typedef struct {
	size_t keyi;
	size_t valuei;
	ssize_t nexti;
} ImdProp;

#define BLOCK_SIZE 256

static size_t
aalloc(MetaString** mstr, size_t size) {
	size = (size + 7) & ~7;
	if((*mstr)->bump_ptr + size > (*mstr)->arena_siz) {
		size_t arena_siz = ((*mstr)->bump_ptr + size + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1);
		if(!(*mstr = (MetaString*)realloc(*mstr, sizeof(MetaString) + arena_siz)))
			die("Could not realloc %zu bytes.", sizeof(MetaString) + arena_siz);
		(*mstr)->arena_siz = arena_siz;
	}
	return ((*mstr)->bump_ptr += size) - size;
}

#define APTR(T, m, p) ((T*)&(m)->arena[p])

static ssize_t
parse_netstring(MetaString** mstr, unsigned* i, const char* raw) {
	unsigned length = 0;
	unsigned begin;
	unsigned j = *i;
	while(raw[j] && raw[j] >= '0' && raw[j] <= '9')
		length = length * 10 + raw[j++] - '0';
	if(raw[j] == ':') {
		begin = ++j;
		while(raw[j] && j-begin < length) ++j;
		if(j != begin + length) {
			return -1;
		}
		*i = j;
	} else {
		j = begin = *i;
		while(raw[j] && raw[j] != '#' && raw[j] != ';' && raw[j] != '=')
			++j;
		length = j - begin;
		*i = j;
	}
	size_t ptr = aalloc(mstr, length + 1);
	memcpy((*mstr)->arena + ptr, raw + begin, length);
	*APTR(char, *mstr, ptr+length) = 0;
	return ptr;
}

MetaString*
parse_metastring(const char* raw) {
	MetaString* mstr = (MetaString*)malloc(sizeof(MetaString));
	if(!mstr) die("Could not malloc %zu bytes.", sizeof(MetaString));
	mstr->bump_ptr = mstr->arena_siz = 0;

	unsigned i = 0;
	ssize_t prop_head = -1;
	ssize_t prop_tail = -1;

	ssize_t contenti = parse_netstring(&mstr, &i, raw);
	if(contenti == -1 || (raw[i] && raw[i++] != '#')) {
		free(mstr);
		return NULL;
	}

	while(raw[i]) {
		ssize_t keyi, valuei;
		if(
			!~(keyi = parse_netstring(&mstr, &i, raw)) ||
			raw[i++] != '=' ||
			!~(valuei = parse_netstring(&mstr, &i, raw)) ||
			(raw[i] && raw[i++] != ';')
		) {
			free(mstr);
			return NULL;
		}
		size_t ptr = aalloc(&mstr, sizeof(MetaStringProperty));
		ImdProp* cprop = APTR(ImdProp, mstr, ptr);
		cprop->keyi = keyi;
		cprop->valuei = valuei;
		cprop->nexti = -1;
		if(prop_tail == -1) {
			prop_tail = prop_head = ptr;
		} else {
			APTR(ImdProp, mstr, prop_tail)->nexti = ptr;
			prop_tail = ptr;
		}
	}

	mstr->content = APTR(char, mstr, contenti);
	if(prop_head == -1) {
		mstr->properties = NULL;
	} else {
		mstr->properties = APTR(MetaStringProperty, mstr, prop_head);
		for(ssize_t ind = prop_head; ~ind;) {
			size_t keyi, valuei;
			ssize_t nexti;
			{
				ImdProp* iprop = APTR(ImdProp, mstr, ind);
				keyi = iprop->keyi, valuei = iprop->valuei;
				nexti = iprop->nexti;
			}
			MetaStringProperty* prop = APTR(MetaStringProperty, mstr, ind);
			prop->key = APTR(char, mstr, keyi);
			prop->value = APTR(char, mstr, valuei);
			prop->next = (~nexti) ? APTR(MetaStringProperty, mstr, nexti) : NULL;
			ind = nexti;

		}
	}
	return mstr;
}

