/*
 * Copyright (c) 2015, Hasso-Plattner-Institut.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "k-means.h"

void buildClusters(const int *data, int nrData, int k, KMeans *p_kmeans) {
	int i, j, changed;
	Cluster clusters[k];

	if (k > nrData) {
		printf("Too few data points");
		return;
	}

	p_kmeans->k = k;
	int centers[k];
	p_kmeans->centers = centers;
	
	// actually take random samples
	for (i = 0; i < k; i++) {
		// Create empty elements array, that could contain all elements.
		// Could be memory-optimized by using realloc or different data structure 
		// for data elements, but as long as we don't have
		// too many clusters or too many data points, this should be fine.
		clusters[i].elements = (int*) malloc(nrData * sizeof(int));
		p_kmeans->centers[i] = data[i];
	}

	changed = 1;
	while (changed) {
		for (i = 0; i < k; i++) {
			clusters[i].nrElements = 0;
		}

		// assign all data values to clusters
		for (i = 0; i < nrData; i++) {
			int datum = data[i];
			int minDist = 10000;
			int bestCluster = -1;
			for (j = 0; j < k; j++) {
				int distance = abs(p_kmeans->centers[j] - datum);
				if (distance < minDist) {
					minDist = distance;
					bestCluster = j;
				}
			}

			clusters[bestCluster].elements[clusters[bestCluster].nrElements] = datum;
			clusters[bestCluster].nrElements++;
		}

		// re-calculate the cluster means
		changed = 0;
		for (i = 0; i < k; i++) {
			int sum = 0;
			for (j = 0; j < clusters[i].nrElements; j++) {
				sum += clusters[i].elements[j];
			}

			// int should be enough here and simplifies the comparison
			int mean = sum / clusters[i].nrElements;
			if (mean != p_kmeans->centers[i]) {
				p_kmeans->centers[i] = mean;
				changed = 1;
			}
		}
	}

	for (i = 0; i < k; i++) {
		printf("mean: %d\n", p_kmeans->centers[i]);
		printf("number of elements: %d: \n", clusters[i].nrElements);
		for (j = 0; j < clusters[i].nrElements; j++) {
			printf("%d ", clusters[i].elements[j]);
		}
		// free the previously allocated memory
		free(clusters[i].elements);
		printf("\n");
	}

	// we should have converged here. yay!
}

int classify(int value, KMeans *p_kmeans) {
	int i, bestCluster = -1, minDist = 10000;

	for (i = 0; i < p_kmeans->k; i++) {
		int dist = abs(p_kmeans->centers[i] - value);
		if (dist < minDist) {
			minDist = dist;
			bestCluster = i;
		}
	}

	return bestCluster;
}