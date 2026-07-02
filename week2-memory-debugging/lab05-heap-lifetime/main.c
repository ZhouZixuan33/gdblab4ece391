#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct request {
    int id;
    char owner[16];
    int state;
};

static struct request *cached_request;

static struct request *create_request(int id, const char *owner) {
    struct request *req = malloc(sizeof(*req));

    if (req == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }

    req->id = id;
    snprintf(req->owner, sizeof(req->owner), "%s", owner);
    req->state = 1;
    return req;
}

static void cache_request(struct request *req) {
    cached_request = req;
}

static void release_request(struct request *req) {
    free(req);
}

static void mark_cached_request_done(void) {
    /*
     * Intentional bug: cached_request still points to the freed heap object.
     * The pointer value looks non-NULL, but the object lifetime is over.
     */
    cached_request->state = 0;
}

static void print_cached_request(const char *label) {
    printf("%s id=%d owner=%s state=%d at %p\n",
           label,
           cached_request->id,
           cached_request->owner,
           cached_request->state,
           (void *)cached_request);
}

static void print_cached_state(const char *label) {
    printf("%s state=%d at %p\n",
           label,
           cached_request->state,
           (void *)cached_request);
}

int main(void) {
    struct request *active = create_request(391, "shell");

    cache_request(active);
    print_cached_request("Before free:");

    release_request(active);
    printf("Released active request, but cached_request still equals %p.\n",
           (void *)cached_request);

    mark_cached_request_done();
    print_cached_state("After stale write:");
    printf("Expected: do not read or write a request after free.\n");

    return 0;
}
