/*
 * Stolen from lib/string.c
 * 
 * The arch dependent code doesn't seem to export these symbols. Very bad.
 */

void *memmove(void * dest,const void *src, unsigned long count)
{
        char *tmp, *s;

        if (dest <= src) {
                tmp = (char *) dest;
                s = (char *) src;
                while (count--)
                        *tmp++ = *s++;
                }
        else {
                tmp = (char *) dest + count;
                s = (char *) src + count;
                while (count--)
                        *--tmp = *--s;
                }

        return dest;
}

long strlen(const char * s)
{
        const char *sc;

        for (sc = s; *sc != '\0'; ++sc)
                /* nothing */;
        return sc - s;
}

