__kernel void vector_addition(const int n,
                              __global const float *a,
                              __global const float *b,
                              __global float *c) {
    // Get the global ID for this thread
    const int i = get_global_id(0);

    // Add the corresponding elements of a and b and store the result in c
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}
