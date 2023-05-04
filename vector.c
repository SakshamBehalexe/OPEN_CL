#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

#define SIZE 1024

// Define the input vectors and their size
float a[SIZE], b[SIZE], c[SIZE];
const int n = SIZE;

// Define the local and global sizes for the OpenCL kernel
const int TS = 256;
const size_t local[1] = { TS };
const size_t global[1] = { (n + TS - 1) / TS * TS };
cl_device_id create_device();
cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename);
// Declare the OpenCL device, context, program, kernel, and command queue
cl_device_id device_id;
cl_context context;
cl_program program;
cl_kernel kernel;
cl_command_queue queue;

// Declare the OpenCL memory objects for the input and output vectors
cl_mem bufA, bufB, bufC;

// Declare an event for timing
cl_event event = NULL;

// Declare an error code variable
int err;





// Define a function to initialize the input vectors
void init(float *v) {
    for (int i = 0; i < n; i++) {
        v[i] = (float)rand() / RAND_MAX;
    }
}



cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename) {

   cl_program program;
   FILE *program_handle;
   char *program_buffer, *program_log;
   size_t program_size, log_size;
  

   /* Read program file and place content into buffer */
   program_handle = fopen(filename, "r");
   if(program_handle == NULL) {
      perror("Couldn't find the program file");
      exit(1);
   }
   fseek(program_handle, 0, SEEK_END);
   program_size = ftell(program_handle);
   rewind(program_handle);
   program_buffer = (char*)malloc(program_size + 1);
   program_buffer[program_size] = '\0';
   fread(program_buffer, sizeof(char), program_size, program_handle);
   fclose(program_handle);

   /* Create program from file 

   Creates a program from the source code in the add_numbers.cl file. 
   Specifically, the code reads the file's content into a char array 
   called program_buffer, and then calls clCreateProgramWithSource.
   */
   program = clCreateProgramWithSource(ctx, 1, 
      (const char**)&program_buffer, &program_size, &err);
   if(err < 0) {
      perror("Couldn't create the program");
      exit(1);
   }
   free(program_buffer);

   /* Build program 

   The fourth parameter accepts options that configure the compilation. 
   These are similar to the flags used by gcc. For example, you can 
   define a macro with the option -DMACRO=VALUE and turn off optimization 
   with -cl-opt-disable.
   */
   err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
   if(err < 0) {

      /* Find size of log and print to std output */
      clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
            0, NULL, &log_size);
      program_log = (char*) malloc(log_size + 1);
      program_log[log_size] = '\0';
      clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
            log_size + 1, program_log, NULL);
      printf("%s\n", program_log);
      free(program_log);
      exit(1);
   }

   return program;
}


cl_device_id create_device() {

   cl_platform_id platform;
   cl_device_id dev;
   int err;

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't identify a platform");
      exit(1);
   } 

   // Access a device
   // GPU
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
      // CPU
      err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
      perror("Couldn't access any devices");
      exit(1);   
   }

   return dev;
}

int main() {

    // Initialize the input vectors
    init(a);
    init(b);

    // Initialize the OpenCL device, context, program, kernel, and command queue
    device_id = create_device();
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    program = build_program(context, device_id, "./vector_addition.cl");
    kernel = clCreateKernel(program, "vector_addition", &err);
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);

    // Allocate memory for the input and output vectors on the OpenCL device
    bufA = clCreateBuffer(context, CL_MEM_READ_ONLY, n * sizeof(float), NULL, NULL);
    bufB = clCreateBuffer(context, CL_MEM_READ_ONLY, n * sizeof(float), NULL, NULL);
    bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, n * sizeof(float), NULL, NULL);

    // Copy the input vectors from the host to the device
    clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, n * sizeof(float), a, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufB, CL_TRUE, 0, n * sizeof(float), b, 0, NULL, NULL);

    // Set the kernel arguments
    clSetKernelArg(kernel, 0, sizeof(int), (void*)&n);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&bufA);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&bufB);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&bufC);

   
// Wait for the kernel to finish executing
clWaitForEvents(1, &event);

// Copy the output vector from the device to the host
clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, n * sizeof(float), c, 0, NULL, NULL);

// Print the first 10 elements of the input and output vectors
printf("Input vectors:\n");
for (int i = 0; i < 10; i++) {
    printf("%f + %f = ", a[i], b[i]);
    printf("%f\n", c[i]);
}

// Release the OpenCL resources
clReleaseMemObject(bufA);
clReleaseMemObject(bufB);
clReleaseMemObject(bufC);
clReleaseKernel(kernel);
clReleaseProgram(program);
clReleaseCommandQueue(queue);
clReleaseContext(context);

return 0;
}