/*
 * Copyright (c) 2012, 2013, 2018 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdio.h>
#include "mtp_descs_strings.h"

static int write_blob(const char *filename, const void *blob, size_t size)
{
	int ret;
	FILE *fp;

	fp = fopen(filename, "w");
	if (!fp) {
		fprintf(stderr, "Could not open %s: %m\n", filename);
		return -errno;
	}

	ret = fwrite(blob, size, 1, fp);
	if (ret < 0) {
		fprintf(stderr, "Could not write to %s\n", filename);
		goto out;
	}

	ret = 0;

out:
	fclose(fp);
	return ret;
}

int main()
{
	int ret;

	ret = write_blob("descs", &descriptors, sizeof(descriptors));
	if (ret < 0)
		return ret;

	ret = write_blob("strs", &strings, sizeof(strings));
	return ret;
}
