import os
from abc import abstractmethod
from pathlib import Path
from typing import (
    List,
    Optional,
    Union,
)

import m5
from m5.util import warn

from ...resources.resource import (
    BootloaderResource,
    CheckpointResource,
    DiskImageResource,
    KernelResource,
)
from .kernel_disk_workload import KernelDiskWorkload
from ...devices.gpus.amdgpu import BaseViperGPU

class GpuFsWorkload(KernelDiskWorkload):

    @abstractmethod
    def set_app_gpu(self, app_gpu: Optional['BaseViperGPU']) -> None:
        """
        Set the GPU device to be used by the application.
        """
        raise NotImplementedError
    
    @abstractmethod
    def get_app_gpu(self) -> Optional['BaseViperGPU']:
        """
        Get the GPU device to be used by the application.
        """
        raise NotImplementedError
    
    def set_gpu_fs_workload(
        self,
        binary: BinaryResource,
        debug: Optional[bool] = False,
        kernel: Optional[KernelResource] = None,
        disk_image: Optional[DiskImageResource] = None,
        bootloader: Optional = None,
        disk_device: Optional[str] = None,
        readfile: Optional[str] = None,
        kernel_args: Optional[List[str]] = None,
        exit_on_work_items: bool = True,
        checkpoint: Optional[Union[Path, str]] = None,
        ) -> None:
        """
        Set the GPU application to be run.
        """
        app_gpu = self.get_app_gpu()
        readfile_contents = None

        if app_gpu is None:
            warn("No GPU device set for the application. GPU device is required.")
        else:
            driver_load_command = self.get_app_gpu().get_driver_load_command(debug=debug)

            binary_path = binary.get_local_path()
            with open(binary_path, "rb") as binfile:
                encodedBin = base64.b64encode(binfile.read()).decode()

            application_command = (
                f'echo "{encodedBin}" | base64 -d > myapp\n'
                "chmod +x myapp\n"
                "./myapp {}\n"
                "/sbin/m5 exit\n"
            )

            readfile_contents = driver_load_command + application_command

        self.set_kernel_disk_workload(
            kernel=kernel,
            disk_image=disk_image,
            bootloader=bootloader,
            disk_device=disk_device,
            readfile=readfile,
            readfile_contents=readfile_contents,
            kernel_args=kernel_args,
            exit_on_work_items=exit_on_work_items,
            checkpoint=checkpoint,
        )
