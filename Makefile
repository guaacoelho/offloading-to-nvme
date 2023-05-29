colon := :
$(colon) := :

DISKS := 8
MPIDISKS := 4
OUTPUT_DIR := results/$(shell /bin/date "+%Y-%m-%d--%H-%M-%S")
OUTPUT_PATH := $(shell realpath $(OUTPUT_DIR))

#SHELL := /bin/bash
nfs: model nfs-disks output-dir 1SOCKET 1SOCKET-CACHE 2SOCKET 2SOCKET-CACHE plot
test: model disks output-dir 1SOCKET plot

# ENVIRONMENT

output-dir:
	mkdir -p $(OUTPUT_DIR)

model: 
	wget -nc ftp://slim.gatech.edu/data/SoftwareRelease/WaveformInversion.jl/3DFWI/overthrust_3D_initial_model.h5

create-env:
	conda env create -f environment.yml

nfs-disks:
	mkdir -p /scr01/data
	$(foreach n,  $(filter-out $(DISKS), $(shell seq 0 $(DISKS))), mkdir -p /scr01/data/nvme$(n);)
	ln -s /scr01/data data


# EXPERIMENTS

1SOCKET: output-dir ram

	mkdir -p $(OUTPUT_DIR)/1SOCKET/forward
	mkdir -p $(OUTPUT_DIR)/1SOCKET/adjoint

	$(MAKE) ram

	@for DISK in $$(seq 1 $(DISKS)); do \
		echo "Running Adjoint to $$DISK disk (s)!"; \
		rm -rf data/nvme*/*; \
		DEVITO_OPT=advanced \
		DEVITO_LANGUAGE=openmp \
		DEVITO_PLATFORM=skx \
		OMP_NUM_THREADS=18 \
		OMP_PLACES="{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17}" \
		DEVITO_LOGGING=DEBUG \
		time numactl --cpubind=0 python3 overthrust_experiment.py --disks=$$DISK; \
	done

	mv fwd*.csv $(OUTPUT_DIR)/1SOCKET/forward
	mv rev*.csv $(OUTPUT_DIR)/1SOCKET/adjoint


1SOCKET-CACHE: create-env model output-dir ram

	mkdir -p $(OUTPUT_DIR)/1SOCKET/cache/forward
	mkdir -p $(OUTPUT_DIR)/1SOCKET/cache/adjoint

	$(MAKE) ram

	@for DISK in $$(seq 1 $(DISKS)); do \
		echo "Running Adjoint CACHED to $$DISK disk (s)!"; \
		rm -rf data/nvme*/*; \
		DEVITO_OPT=advanced \
		DEVITO_LANGUAGE=openmp \
		DEVITO_PLATFORM=skx \
		OMP_NUM_THREADS=18 \
		OMP_PLACES="{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17}" \
		DEVITO_LOGGING=DEBUG \
		time numactl --cpubind=0 python3 overthrust_experiment.py --disks=$$DISK --cache; \
	done

	mv fwd*.csv $(OUTPUT_DIR)/1SOCKET/cache/forward
	mv rev*.csv $(OUTPUT_DIR)/1SOCKET/cache/adjoint

# Missing --bind-to socket
2SOCKET: create-env model output-dir ram-mpi

	mkdir -p $(OUTPUT_DIR)/2SOCKET/forward
	mkdir -p $(OUTPUT_DIR)/2SOCKET/adjoint

	$(MAKE) ram-mpi

	@for DISK in $$(seq 1 $(MPIDISKS)); do \
		echo "Running Adjoint CACHED to $$DISK disk (s)!"; \
		rm -rf data/nvme*/*; \
		DEVITO_OPT=advanced \
		DEVITO_LANGUAGE=openmp \
		DEVITO_MPI=1 \
		OMP_NUM_THREADS=18 \
		OMP_PLACES="{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35}" \
		DEVITO_LOGGING=DEBUG \
		time mpirun --allow-run-as-root --map-by socket -np 2 python3 overthrust_experiment.py --mpi --disks=$$DISK; \
	done

	mv fwd*.csv $(OUTPUT_DIR)/2SOCKET/forward
	mv rev*.csv $(OUTPUT_DIR)/2SOCKET/adjoint

# Missing --bind-to socket
2SOCKET-CACHE: create-env model output-dir ram-mpi

	mkdir -p $(OUTPUT_DIR)/2SOCKET/cache/forward
	mkdir -p $(OUTPUT_DIR)/2SOCKET/cache/adjoint

	$(MAKE) ram-mpi

	@for DISK in $$(seq 1 $(MPIDISKS)); do \
		echo "Running Adjoint CACHED to $$DISK disk (s)!"; \
		rm -rf data/nvme*/*; \
		DEVITO_OPT=advanced \
		DEVITO_LANGUAGE=openmp \
		DEVITO_MPI=1 \
		OMP_NUM_THREADS=18 \
		OMP_PLACES="{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35}" \
		DEVITO_LOGGING=DEBUG \
		time mpirun --allow-run-as-root --map-by socket -np 2 python3 overthrust_experiment.py --cache --mpi --disks=$$DISK; \
	done

	mv fwd*.csv $(OUTPUT_DIR)/2SOCKET/cache/forward
	mv rev*.csv $(OUTPUT_DIR)/2SOCKET/cache/adjoint

test: create-env model
	rm -rf data/nvme*/*
	DEVITO_OPT=advanced \
	DEVITO_LANGUAGE=openmp \
	DEVITO_PLATFORM=skx \
	OMP_NUM_THREADS=18 \
	OMP_PLACES="{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17}" \
	DEVITO_LOGGING=DEBUG \
	time numactl --cpubind=0  python3 tests/gradient_test.py

ram: 
	rm -rf data/nvme*/*
	DEVITO_OPT=advanced \
	DEVITO_LANGUAGE=openmp \
	DEVITO_PLATFORM=skx \
	OMP_NUM_THREADS=18 \
	OMP_PLACES="{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17}" \
	DEVITO_LOGGING=DEBUG \
	time numactl --cpubind=0  python3 overthrust_experiment.py --ram

# Missing --bind-to socket
ram-mpi: create-env model
	rm -rf data/nvme*/*
	DEVITO_OPT=advanced \
	DEVITO_LANGUAGE=openmp \
	DEVITO_MPI=1 \
	OMP_NUM_THREADS=18 \
	OMP_PLACES="{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35}" \
	DEVITO_LOGGING=DEBUG \
	OMPI_ALLOW_RUN_AS_ROOT=1 \
	time mpirun --allow-run-as-root --map-by socket -np 2 python3 overthrust_experiment.py --mpi --ram


## PLOT RESULTS ##
plots:
	python3 plot/generate.py --path=$(OUTPUT_PATH)

clean:
	rm -rf results
