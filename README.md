
         static int use_ion_dmabuf_flag = 0;



                        { .name = "use_ion_dmabuf",             .has_arg = 0, .flag = &use_ion_dmabuf_flag, .val = 1},


                                if (use_ion_dmabuf_flag) {
                                        user_param->use_ion_dmabuf = 1;
                                        user_param->memory_create = ion_memory_create;
#if 0
                                        if (user_param->memory_type != MEMORY_NEURON) {
                                                fprintf(stderr, "ion DMA-BUF cannot be used \n");
                                                free(duplicates_checker);
                                                return FAILURE;
                                        }
#endif
                                        use_ion_dmabuf_flag = 0;
                                }

         src/ion_memory.c src/raw_ethernet_resources.c
 am__dirstamp = $(am__leading_dot)dirstamp
 #am__objects_1 = src/cuda_memory.$(OBJEXT)
 #am__objects_2 = src/rocm_memory.$(OBJEXT)
 #am__objects_3 = src/neuron_memory.$(OBJEXT)
 #am__objects_4 = src/hl_memory.$(OBJEXT)
 am__objects_5 =  \
         src/raw_ethernet_resources.$(OBJEXT)
 am__objects_6 =
 am_libperftest_a_OBJECTS = src/get_clock.$(OBJEXT) \
         src/perftest_communication.$(OBJEXT) \
         src/perftest_parameters.$(OBJEXT) \
         src/perftest_resources.$(OBJEXT) \
         src/perftest_counters.$(OBJEXT) src/host_memory.$(OBJEXT) \
         src/mmap_memory.$(OBJEXT) $(am__objects_1) $(am__objects_2) \
         $(am__objects_3) $(am__objects_4) src/ion_memory.$(OBJEXT) \

$ sudo ./ib_send_bw -s 2097152  -n 100 192.168.1.1 --use_ion_dmabuf

inside ion_memory_create
ion alloc success: size = 4194304, dmabuf_fd = 7
Calling ibv_reg_dmabuf_mr(offset=0, size=4194304, addr=0x7fe88de00000, fd=7) for QP #0
Total number of CQs created :2
