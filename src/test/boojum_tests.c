#include "boojum_tests.h"
#include <boojum.h>
#if defined(_WIN32)
# include <windows.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

static void print_segment_data(const void *segment, const size_t segment_size);

// WARN(Rafael): libcutest's memory leak detector have been disabled for all main boojum's entry points
//               because it depends on pthread conveniences and it by default leaks some resources even
//               when taking care of requesting it for do not leak nothing... Anyway, all btree tests
//               are being executed with memory leak detector turned on. In Boojum this is the main
//               spot where memory leak could be harmful.
//
//               WINAPI also leaks some resources related to multi-threading stuff.

CUTE_TEST_CASE(boojum_init_tests)
    int status = g_cute_leak_check;
    CUTE_ASSERT(boojum_init(0) == EINVAL);
    g_cute_leak_check = 0;
    CUTE_ASSERT(boojum_init(1000) == EXIT_SUCCESS);
    CUTE_ASSERT(boojum_init(1000) == EXIT_SUCCESS);
    CUTE_ASSERT(boojum_deinit() == EXIT_SUCCESS);
    g_cute_leak_check = status;
CUTE_TEST_CASE_END

CUTE_TEST_CASE(boojum_deinit_tests)
    int status = g_cute_leak_check;
    CUTE_ASSERT(boojum_deinit() == EINVAL);
    g_cute_leak_check = 0;
    CUTE_ASSERT(boojum_init(1000) == EXIT_SUCCESS);
    CUTE_ASSERT(boojum_deinit() == EXIT_SUCCESS);
    g_cute_leak_check = status;
CUTE_TEST_CASE_END

CUTE_TEST_CASE(boojum_alloc_free_tests)
    int status = g_cute_leak_check;
    char *segment = NULL;
    g_cute_leak_check = 0;
    segment = boojum_alloc(1024);
    CUTE_ASSERT(segment == NULL);
    CUTE_ASSERT(boojum_init(1000) == EXIT_SUCCESS);
    segment = boojum_alloc(1024);
    CUTE_ASSERT(segment != NULL);
    CUTE_ASSERT(boojum_free(segment) == EXIT_SUCCESS);
    CUTE_ASSERT(boojum_deinit() == EXIT_SUCCESS);
    g_cute_leak_check = status;
CUTE_TEST_CASE_END

CUTE_TEST_CASE(boojum_set_get_tests)
    int status = g_cute_leak_check;
    char *segment = NULL;
    char *plain = NULL;
    size_t plain_size = 0;
    g_cute_leak_check = 0;
    CUTE_ASSERT(boojum_init(1000) == EXIT_SUCCESS);
    segment = boojum_alloc(6);
    CUTE_ASSERT(segment != NULL);
    plain_size = 6;
    plain = (char *)malloc(plain_size);
    CUTE_ASSERT(plain != NULL);
    memcpy(plain, "foobar", plain_size);
    CUTE_ASSERT(boojum_set(segment, plain, &plain_size) == EXIT_SUCCESS);
    CUTE_ASSERT(plain_size == 0);
    CUTE_ASSERT(memcmp(plain, "foobar", 6) != 0);
    free(plain);
    CUTE_ASSERT(memcmp(segment, "foobar", 6) != 0);
    plain = boojum_get(segment, &plain_size);
    CUTE_ASSERT(plain != NULL);
    CUTE_ASSERT(plain_size == 6);
    CUTE_ASSERT(memcmp(plain, "foobar", 6) == 0);
    free(plain);
    // INFO(Rafael): Since it is a C library it is up to users free everything they have allocated.
    CUTE_ASSERT(boojum_free(segment) == EXIT_SUCCESS);
    CUTE_ASSERT(boojum_deinit() == EXIT_SUCCESS);
    g_cute_leak_check = status;
CUTE_TEST_CASE_END

CUTE_TEST_CASE(boojum_alloc_realloc_free_tests)
    int status = g_cute_leak_check;
    char *old = NULL, *new = NULL;
    char *data = NULL;
    size_t data_size = 0;
    g_cute_leak_check = 0;
    CUTE_ASSERT(boojum_init(1000) == EXIT_SUCCESS);
    old = boojum_alloc(3);
    CUTE_ASSERT(old != NULL);
    CUTE_ASSERT(boojum_realloc(NULL, 6) == NULL);
    CUTE_ASSERT(boojum_realloc(old, 0) == NULL);
    new = boojum_realloc(old, 6);
    CUTE_ASSERT(new != NULL && new != old);
    CUTE_ASSERT(boojum_free(new) == EXIT_SUCCESS);
    data = (char *)malloc(3);
    CUTE_ASSERT(data != NULL);
    memcpy(data, "foo", 3);
    data_size = 3;
    old = boojum_alloc(3);
    CUTE_ASSERT(old != NULL);
    CUTE_ASSERT(boojum_set(old, data, &data_size) == EXIT_SUCCESS);
    free(data);
    new = boojum_realloc(old, 6);
    CUTE_ASSERT(new != NULL && new != old);
    data = boojum_get(new, &data_size);
    CUTE_ASSERT(data != NULL);
    CUTE_ASSERT(data_size == 3);
    CUTE_ASSERT(memcmp(data, "foo", 3) == 0);
    free(data);
    data = (char *)malloc(6);
    CUTE_ASSERT(data != NULL);
    memcpy(data, "foobar", 6);
    data_size = 6;
    CUTE_ASSERT(boojum_set(new, data, &data_size) == EXIT_SUCCESS);
    free(data);
    data = boojum_get(new, &data_size);
    CUTE_ASSERT(data != NULL);
    CUTE_ASSERT(data_size == 6);
    CUTE_ASSERT(memcmp(data, "foobar", 6) == 0);
    free(data);
    CUTE_ASSERT(boojum_free(new) == EXIT_SUCCESS);
    CUTE_ASSERT(boojum_deinit() == EXIT_SUCCESS);
    g_cute_leak_check = status;
CUTE_TEST_CASE_END

CUTE_TEST_CASE(boojum_set_timed_get_tests)
    char *data = NULL;
    size_t data_size = 0;
    char *segment = NULL;
    int status = g_cute_leak_check;
    g_cute_leak_check = 0;
    CUTE_ASSERT(boojum_init(1000) == EXIT_SUCCESS);
    segment = boojum_alloc(6);
    CUTE_ASSERT(segment != NULL);
    data = (char *)malloc(6);
    CUTE_ASSERT(data != NULL);
    data_size = 6;
    memcpy(data, "secret", data_size);
    CUTE_ASSERT(boojum_set(segment, data, &data_size) == EXIT_SUCCESS);
    CUTE_ASSERT(data_size == 0);
    CUTE_ASSERT(memcmp(data, "\x0\x0\x0\x0\x0\x0", 6) == 0);
    free(data);
    data = boojum_timed_get(segment, &data_size, 1000);
    CUTE_ASSERT(data_size == 6);
    CUTE_ASSERT(data != NULL);
    CUTE_ASSERT(memcmp(data, "secret", data_size) == 0);
#if defined(__unix__)
    sleep(3);
#elif defined(_WIN32)
    Sleep(3000);
#else
# error Some code wanted.
#endif
    CUTE_ASSERT(data_size == 0);
    CUTE_ASSERT(boojum_free(segment) == EXIT_SUCCESS);
    CUTE_ASSERT(boojum_deinit() == EXIT_SUCCESS);
    g_cute_leak_check = status;
CUTE_TEST_CASE_END

CUTE_TEST_CASE(boojum_kupd_assurance_tests)
#define MAS_EH_CLARO "S.B.B.H.K.K! S.B.B.H.K.K! Aqui � o Chapolin Colorado falando � Terra!"
    char *segment = NULL;
    char *data = NULL;
    size_t data_size;
    const size_t eavesdrop_attempts_nr = 50;
    int status = g_cute_leak_check;
    size_t e;
    g_cute_leak_check = 0;
    CUTE_ASSERT(boojum_init(1000) == EXIT_SUCCESS);
    data_size = strlen(MAS_EH_CLARO);
    data = (char *)malloc(data_size);
    CUTE_ASSERT(data != NULL);
    memcpy(data, MAS_EH_CLARO, data_size);
    segment = boojum_alloc(data_size);
    CUTE_ASSERT(segment != NULL);
    CUTE_ASSERT(boojum_set(segment, data, &data_size) == EXIT_SUCCESS);
    CUTE_ASSERT(data_size == 0);
    CUTE_ASSERT(data[0] == 0);
    data_size = strlen(MAS_EH_CLARO);
    fprintf(stdout, "Now eavesdropping masked segment... wait...");
    for (e = 0; e < eavesdrop_attempts_nr; e++) {
        CUTE_ASSERT(memcmp(segment, MAS_EH_CLARO, data_size) != 0);
        memcpy(data, segment, data_size);
        sleep(2);
        CUTE_ASSERT(memcmp(segment, data, data_size) != 0);
        fprintf(stdout, "\r                                                                                 "
                        "                            \r"
                        "[%2.f%% completed]", (((double)e + 1) / (double)eavesdrop_attempts_nr) * 100);
        fprintf(stdout, " Masked memory segment's last status: ");
        print_segment_data(data, 8);

    }
    fprintf(stdout, "\r                                                                                     "
                    "                               \r");
    free(data);
    data = NULL;
    CUTE_ASSERT(boojum_free(segment) == EXIT_SUCCESS);
    CUTE_ASSERT(boojum_deinit() == EXIT_SUCCESS);
    g_cute_leak_check = status;
#undef MAS_EH_CLARO
CUTE_TEST_CASE_END

static void print_segment_data(const void *segment, const size_t segment_size) {
    const unsigned char *sp = (const unsigned char *)segment;
    const unsigned char *sp_end = sp + segment_size;
    const char token[2] = { 0, ',' };
#if defined(_WIN32)
    fprintf(stdout, " 0x%p..%.2X = { ", segment, ((uintptr_t)segment & 0xFF) + segment_size);
#else
    fprintf(stdout, " %p..%.2x = { ", segment, ((uintptr_t)segment & 0xFF) + segment_size);
#endif
    while (sp != sp_end) {
        fprintf(stdout, "%.2x%c ", *sp, token[(sp + 1) != sp_end]);
        sp++;
    }
    fprintf(stdout, "};");
}
