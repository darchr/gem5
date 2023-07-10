# Copyright (c) 2023 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from abc import ABC, abstractmethod
from typing import Any, Dict, List, Optional
import urllib.parse


class AbstractClient(ABC):
    def verify_status_code(self, status_code: int) -> None:
        """
        Verifies that the status code is 200.
        :param status_code: The status code to verify.
        """
        if status_code == 200:
            return
        if status_code == 429:
            raise Exception("Panic: Too many requests")
        if status_code == 401:
            raise Exception("Panic: Unauthorized")
        if status_code == 404:
            raise Exception("Panic: Not found")
        if status_code == 400:
            raise Exception("Panic: Bad request")
        if status_code == 500:
            raise Exception("Panic: Internal server error")

        raise Exception(f"Panic: Unknown status code {status_code}")

    def _url_validator(self, url: str) -> bool:
        """
        Validates the provided URL.
        :param url: The URL to be validated.
        :return: True if the URL is valid, False otherwise.
        """
        try:
            result = urllib.parse.urlparse(url)
            return all([result.scheme, result.netloc, result.path])
        except:
            return False

    @abstractmethod
    def get_resources(
        self,
        resource_id: Optional[str] = None,
        resource_version: Optional[str] = None,
        gem5_version: Optional[str] = None,
    ) -> List[Dict[str, Any]]:
        """
        :param resource_id: The ID of the Resource. Optional, if not set, all
        resources will be returned.
        :param resource_version: The version of the Resource. Optional, if
        not set, all resource versions will be returned. Note: If `resource_id`
        is not set, this parameter will be ignored.
        :param gem5_version: The version of gem5. Optional, if not set, all
        versions will be returned.
        :return: A list of all the Resources with the given ID.
        """
        raise NotImplementedError

    def filter_incompatible_resources(
        self,
        resources_to_filter: List[Dict[str, Any]],
        gem5_version: Optional[str] = None,
    ) -> List[Dict[str, Any]]:
        """Returns a filtered list resources based on gem5 version
        compatibility.

        Note: This function assumes if the minor component of
        a resource's gem5_version is not specified, the resource is compatible
        with all minor versions of the same major version.
        Likewise, if no hot-fix component is specified, it is assumed that
        the resource is compatible with all hot-fix versions of the same
        minor version.

        * '20.1' would be compatible with gem5 '20.1.1.0' and '20.1.2.0'.
        * '21.5.2' would be compatible with gem5 '21.5.2.0' and '21.5.2.0'.
        * '22.3.2.4' would only be compatible with gem5 '22.3.2.4'.

        :param resources_to_filter: The list of resources to filter.
        :param gem5_version: The gem5 version in which the filtered resources
        should be compatible. If None, no filtering will be done.
        :
        """
        if not gem5_version:
            return resources_to_filter

        filtered_resources = []
        for resource in resources_to_filter:
            for version in resource["gem5_versions"]:
                if gem5_version.startswith(version):
                    filtered_resources.append(resource)
        return filtered_resources

    def get_resources_by_id(self, resource_id: str) -> List[Dict[str, Any]]:
        """
        :param resource_id: The ID of the Resource.
        :return: A list of all the Resources with the given ID.
        """
        return self.get_resources(resource_id=resource_id)
