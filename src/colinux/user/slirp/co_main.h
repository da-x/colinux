
#ifdef _WIN32
 #define strncasecmp memicmp
#endif

co_rc_t co_slirp_mutex_init (void);
void co_slirp_mutex_destroy (void);
void co_slirp_mutex_lock (void);
void co_slirp_mutex_unlock (void);

co_rc_t co_slirp_main(int argc, char *argv[]);
