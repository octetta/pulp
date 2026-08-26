void skode_stream_copy(void *ctx, int dst, int src) {
    if (dst < 0 || dst >= 128 || src < 0 || src >= 128) return;
    skode_stream_t *s_dst = &global_stream[dst];
    skode_stream_t *s_src = &global_stream[src];
    s_dst->len = s_src->len;
    s_dst->mode = s_src->mode;
    s_dst->pos = s_src->pos;
    s_dst->dir = s_src->dir;
    if (s_src->len > 0) {
        memcpy(s_dst->data, s_src->data, s_src->len * sizeof(double));
    }
}
