#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//使用するには専用有限要素解析ソフトで作成されたファイルが必要になります

typedef  struct  __node_t_ {
  double  *x;
} NODE_t;

typedef  struct  __element_t_ {
  int  *node;
  int  fracture;
} ELEMENT_t;

typedef  struct  __mesh_t_ {
  int  nNodes;
  NODE_t  *node;
  int  nElements;
  ELEMENT_t  *element;
} MESH_t;

static  void  ReadMesh
(MESH_t *mesh, char *directory)
{
  int  iElement;
  int  iNode;
  int  iDim;
  char  file_name[256];
  FILE  *fp;

  sprintf (file_name, "%s/mesh.inp", directory);
  fp = fopen (file_name, "r");

  fscanf (fp, "%d%d%*d%*d",
          &mesh->nElements, &mesh->nNodes);

  mesh->element = (ELEMENT_t*)malloc (mesh->nElements*sizeof (ELEMENT_t));
  mesh->node = (NODE_t*)malloc (mesh->nElements*sizeof (NODE_t));

  for (iNode = 0; iNode < mesh->nNodes; iNode++) {
    mesh->node[iNode].x = (double*)malloc (3*sizeof (double));
  }

  for (iElement = 0; iElement < mesh->nElements; iElement++) {
    ELEMENT_t  *element = &mesh->element[iElement];

    element->node = (int*)malloc (20*sizeof (int));
    for (iNode = 0; iNode < 20; iNode++) {
      int  nodeID;

      fscanf (fp, "%d", &nodeID);
      element->node[iNode] = nodeID - 1;
    }
  }

  for (iDim = 0; iDim < 3; iDim++) {
    for (iNode = 0; iNode < mesh->nNodes; iNode++) {
      fscanf (fp, "%lf", &mesh->node[iNode].x[iDim]);
    }
  }

  fclose (fp);
}

static  int  ReadFracture
(int stepID, MESH_t *mesh, char *directory)
{
  int  iElement;
  char  file_name[256];
  FILE  *fp;

  sprintf (file_name, "%s/fracture/fracture_%d.out", directory, stepID);
  fp = fopen (file_name, "r");

  if (fp == NULL) return  0;

  for (iElement = 0; iElement < mesh->nElements; iElement++) {
    int  iIntegral;
    int  fracture = 0;

    for (iIntegral = 0; iIntegral < 27; iIntegral++) {
      int  fr;

      fscanf (fp, "%d", &fr);
      fracture += fr;
    }

    mesh->element[iElement].fracture = fracture;
  }

  fclose (fp);

  return  1;
}

//８点平均亀裂長さ
//一応nDivisionsで8点以外もできる

static  void  MeasureCrackLength
(int stepID, MESH_t *mesh, double thickness, int nDivisions, FILE *fp)
{
  int  i;
  double  *a = (double*)malloc (nDivisions*sizeof (double));
  double  a_avg = 0.0;
  double  a0 = 20.0;
  double  dz = thickness / (double)nDivisions;

  for (i = 0; i < nDivisions; i++) {//for1
    int  iElement;
    double  z = ((double)i + 0.5)*dz;

    a[i] = a0;
    for (iElement = 0; iElement < mesh->nElements; iElement++) {//for2
      int  iNode;
      ELEMENT_t  *element = &mesh->element[iElement];
      double  element_y = 0.0;
      double  element_z = 0.0;

      if (element->fracture == 0) continue;

      for (iNode = 0; iNode < 20; iNode++) 
        int  nodeID = element->node[iNode];

        element_y += mesh->node[nodeID].x[1];
	element_z += mesh->node[nodeID].x[2];
      }
      element_y /= 20.0;
      element_z /= 20.0;

      if (fabs (z - element_z) < 0.25*dz) {//後で見直す
        if (a[i] > element_y) {
	  a[i] = element_y;
	}
      }
    }
  }

  for (i = 0; i < nDivisions; i++) {//for4
    a_avg += a[i];
  }
  a_avg /= (double)nDivisions;

  free (a);

  fprintf (fp, "%d %e\n", stepID, a0 - a_avg);
}

int  main (int ac, char **av)
{
  int  stepID = 0;
  MESH_t  mesh;
  double  thickness = atof (av[2]);
  int  nDivisions = atoi (av[3]);
  char  file_name[256];
  FILE  *fp;

  if (ac != 4) {
    fprintf (stderr, "Usage: %s [working dir.] [thickness] [nDivisions]\n", av[0]);
    return  1;
  }

  ReadMesh (&mesh, av[1]);

  sprintf (file_name, "%s/crack_growth_amount.out", av[1]);
  fp = fopen (file_name, "w");
  while (ReadFracture (stepID, &mesh, av[1])) {
    MeasureCrackLength (stepID, &mesh, thickness, nDivisions, fp);
    stepID++;
  }

  fclose (fp);

  return  0;
}
