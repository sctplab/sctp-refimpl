#define REGISTER_APITEST(test) {#test, test}
#define DECLARE_APITEST(test) extern char * test(void)
#define DEFINE_APITEST(test) char *test(void)

struct test {
	char *name;
	char *(*func)(void);
};
