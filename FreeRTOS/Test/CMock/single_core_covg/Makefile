# Indent with spaces
.RECIPEPREFIX := $(.RECIPEPREFIX) $(.RECIPEPREFIX)
# Do not move this line below the include
MAKEFILE_ABSPATH     := $(abspath $(lastword $(MAKEFILE_LIST)))
include ../makefile.in

# SUITES lists the suites contained in subdirectories of this directory
SUITES	+=	single_core_idle_task_thread_covg


# PROJECT and SUITE variables are determined based on path like so:
#   $(UT_ROOT_DIR)/$(PROJECT)/$(SUITE)
PROJECT :=  $(lastword $(subst /, ,$(dir $(abspath $(MAKEFILE_ABSPATH)))))

include ../subdir.mk
