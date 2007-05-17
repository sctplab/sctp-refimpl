#define REGISTER_APITEST(suite, case) {0, #suite"_"#case, #suite, suite##case}
#define DECLARE_APITEST(suite, case) extern char *suite##case(void)
#define DEFINE_APITEST(suite, case) char *suite##case(void)

struct test {
	int enabled;
	char *case_name;
	char *suite_name;
	char *(*func)(void);
};
