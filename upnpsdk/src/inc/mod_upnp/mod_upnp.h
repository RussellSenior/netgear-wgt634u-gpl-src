#ifndef __MOD_UPNP_H__
# define __MOD_UPNP_H__

# ifdef __cplusplus
extern "C" {
# endif

int MiniServerInit(char *webRoot);
int MiniServerDeInit();

typedef struct CBParams{
	int cmdType;
	int rec;
}CBParams;

# ifdef __cplusplus
}
# endif

#endif
