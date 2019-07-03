/*******************************************************************************
  sinqhmsrv.c
*******************************************************************************/

/**
 * This file belongs to the SINQ histogram memory implementation for
 * LINUX-RTAI. This files protoypes several functions useful for the
 * implementation of the control server.
 *
 * Mark Koennecke, Gerd Theidel, Spetember 2005
 */

/*******************************************************************************
  includes
*******************************************************************************/

#include "sinqhmsrv.h"
#include "controlshm.h"
#include "datashm.h"
#include "sinqhm_errors.h"
#include "stptok.h"
#include <mxml.h>
#include <time.h>

/*******************************************************************************
  function declarations
*******************************************************************************/
/**
 * FUNCTION
 *   readXMLData
 * DESCRIPTION
 *   reads a block of XML integer data into iPtr
 *****************************************************************************/
static int readXMLData(mxml_node_t *startNode, int length, uint *iPtr) {
  int i, count = 0;
  mxml_node_t *current = NULL;

  current = startNode;
  for (i = 0; i < length; i++) {
    if (current != NULL && current->type == MXML_INTEGER) {
      iPtr[count] = current->value.integer;
      count++;
    }
    if (current != NULL) {
      current = current->next;
    }
  }
  return count;
}
/******************************************************************************
 * FUNCTION:
 *   loadDataArray
 * DESCRIPTION
 *   try to find and load another array with axis data
 ****************************************************************************/
static int loadDataArray(mxml_node_t *current, array_descr_type *ar,
                         const char *name) {

  mxml_node_t *array = NULL;
  const char *attribute = NULL;
  char *dim = NULL;
  int count, length, i;

  /*
    skip to the root of the node tree
  */
  while (current->parent != NULL) {
    current = current->parent;
  }

  /*
    locate array element
  */
  array = mxmlFindElement(current, current, name, NULL, NULL, MXML_DESCEND);
  if (array == NULL) {
    return 0;
  }

  /*
    get rank
  */
  attribute = mxmlElementGetAttr(array, "rank");
  if (attribute == NULL) {
    return NOARRAYRANK;
  }
  ar->rank = atoi(attribute);
  ar->dim = (uint32 *)malloc(ar->rank * sizeof(uint32));
  if (ar->dim == NULL) {
    return NOMEMORY;
  }
  memset(ar->dim, 0, ar->rank * sizeof(uint32));

  /*
    read dimensions
  */
  attribute = mxmlElementGetAttr(array, "dim");
  if (attribute == NULL) {
    return NOARRAYDIM;
  }
  dim = strtok((char *)attribute, ",");
  if (dim != NULL) {
    ar->dim[0] = atoi(dim);
    count = 1;
  } else {
    count = 0;
  }
  while ((dim = strtok(NULL, ",")) != NULL && count < ar->rank) {
    ar->dim[count] = atoi(dim);
    count++;
  }
  if (count < ar->rank) {
    return NOTENOUGHDIM;
  }

  length = ar->dim[0];
  for (i = 1; i < ar->rank; i++) {
    length *= ar->dim[i];
  }

  ar->data = (uint32 *)malloc(length * sizeof(uint32));
  if (ar->data == NULL) {
    return NOMEMORY;
  }
  memset(ar->data, 0, length * sizeof(uint32));
  if (readXMLData(array->child, length, ar->data) != length) {
    return ARRAYSHORT;
  }
  return 1;
}
/*****************************************************************************/

static int parseAxisArray(mxml_node_t *axisNode, array_descr_type *ardesc) {
  const char *attribute = NULL;
  int length, i, status;

  attribute = mxmlElementGetAttr(axisNode, "array");
  if (attribute == NULL) {
    return ARRAYMISSING;
  } else {
    memset(ardesc, 0, sizeof(array_descr_type));
    status = loadDataArray(axisNode, ardesc, attribute);
    if (status != 1) {
      return status;
    }
  }
  length = ardesc->dim[0];
  for (i = 1; i < ardesc->rank; i++) {
    length *= ardesc->dim[i];
  }
  return length;
}
/*******************************************************************************
 *
 * FUNCTION
 *   parseAxis
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int parseAxis(histo_descr_type *pHM, int noBank, int axis,
              mxml_node_t *axisNode) {
  const char *attribute = NULL;
  int length;
  array_descr_type *ardesc;

  /*
   * find length
   */
  attribute = mxmlElementGetAttr(axisNode, "length");
  if (attribute == NULL) {
    return NOAXISLENGTH;
  }
  length = atoi(attribute);
  pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].length = atoi(attribute);
  if (length < 1) {
    return BADLENGTH;
  }

  /*
   * find mapping
   */

  attribute = mxmlElementGetAttr(axisNode, "mapping");
  if (attribute == NULL) {
    return NOAXISMAPPING;
  }

  if (strcmp(attribute, "direct") == 0) {
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].type = AXDIRECT;
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr = NULL;
    return 1;
  } else if (strcmp(attribute, "calculate") == 0) {
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].type = AXCALC;
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr =
        (uint *)malloc(4 * sizeof(int));
    if (pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr ==
        NULL) {
      return NOMEMORY;
    }
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr[0] = 0;
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr[1] = 0;
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr[2] = 0;
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr[3] = 0;

    attribute = mxmlElementGetAttr(axisNode, "multiplier");
    if (attribute) {
      pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr[0] =
          atoi(attribute);
    }

    attribute = mxmlElementGetAttr(axisNode, "preoffset");
    if (attribute) {
      pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr[1] =
          atoi(attribute);
    }

    attribute = mxmlElementGetAttr(axisNode, "divisor");
    if (attribute) {
      pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr[2] =
          atoi(attribute);
    }

    attribute = mxmlElementGetAttr(axisNode, "postoffset");
    if (attribute) {
      pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr[3] =
          atoi(attribute);
    }

    return 1;
  } else if (strcmp(attribute, "boundary") == 0) {
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].type = AXBOUNDARY;
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].threshold = 0;
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].offset = 0;

    attribute = mxmlElementGetAttr(axisNode, "threshold");
    if (attribute) {
      pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].threshold =
          atoi(attribute);
    }

    attribute = mxmlElementGetAttr(axisNode, "offset");
    if (attribute) {
      pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].offset = atoi(attribute);
    }

    ardesc = (array_descr_type *)malloc(sizeof(array_descr_type));
    if (ardesc == NULL) {
      return NOMEMORY;
    }
    length = parseAxisArray(axisNode, ardesc);
    if (length > 0) {
      pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr =
          (uint32 *)ardesc;
      //      pHM->length += length + 1 + ardesc->rank;
      return 1;
    } else {
      return length;
    }
  } else if (strcmp(attribute, "lookuptable") == 0) {
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].type = AXLOOKUP;
    ardesc = (array_descr_type *)malloc(sizeof(array_descr_type));
    if (ardesc == NULL) {
      return NOMEMORY;
    }
    length = parseAxisArray(axisNode, ardesc);
    if (length > 0) {
      pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr =
          (uint32 *)ardesc;
      //      pHM->length += length + 1 + ardesc->rank;
      return 1;
    } else {
      return length;
    }
  }

  //  \author Michele Brambilla <michele.brambilla@psi.ch>
  //  \date Tue Apr 11 15:15:39 2017
  else if (strcmp(attribute, "map") == 0) {
    pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].type = AXMAP;
    ardesc = (array_descr_type *)malloc(sizeof(array_descr_type));
    if (ardesc == NULL) {
      return NOMEMORY;
    }
    length = parseAxisArray(axisNode, ardesc);
    if (length > 0) {
      pHM->bank_descr.ptr[noBank].axis_descr.ptr[axis].axis_data.ptr =
          (uint32 *)ardesc;
      //      pHM->length += length + 1 + ardesc->rank;
      return 1;
    } else {
      return length;
    }
  } else {
    return BADMAPPING;
  }

  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   parseBank
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int parseBank(histo_descr_type *pHM, int noBank, mxml_node_t *bank) {
  mxml_node_t *axisNode = NULL;
  int naxis = 0, i = 0;
  int status;

  /**
   * count axis
   */
  axisNode = bank;
  while ((axisNode = mxmlFindElement(axisNode, bank, "axis", NULL, NULL,
                                     MXML_DESCEND)) != NULL) {
    naxis++;
  }
  if (naxis < 1) {
    return NOAXIS;
  }

  /**
   * allocate axis space
   */
  pHM->bank_descr.ptr[noBank].rank = naxis;
  pHM->bank_descr.ptr[noBank].axis_descr.ptr =
      (axis_descr_type *)malloc(naxis * sizeof(axis_descr_type));
  if (pHM->bank_descr.ptr[noBank].axis_descr.ptr == NULL) {
    return NOMEMORY;
  }
  memset(pHM->bank_descr.ptr[noBank].axis_descr.ptr, 0,
         naxis * sizeof(axis_descr_type));
  //  pHM->length += naxis*3;                                   /* length, type,
  //  offset per axis */

  /**
   * parseAxis
   */
  for (i = 0, axisNode = bank; i < naxis; i++) {
    axisNode =
        mxmlFindElement(axisNode, bank, "axis", NULL, NULL, MXML_DESCEND);
    status = parseAxis(pHM, noBank, i, axisNode);
    if (status < 0) {
      return status;
    }
  }

  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   parseConfiguration
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int parseConfiguration(histo_descr_type *pHM, mxml_node_t *root) {
  mxml_node_t *current = NULL, *count = NULL;
  const char *filler = NULL;
  const char *attribute = NULL;
  array_descr_type *ardesc;

  int nbank = 0, i, length;
  int status;
  unsigned int ui;

  pHM->nBank = 0;
  //  pHM->length = 0;
  pHM->bank_descr.ptr = NULL;
  pHM->histo_type = 0;
  current = root;

  current = mxmlFindElement(root, root, "sinqhm", NULL, NULL, MXML_DESCEND);
  if (current == NULL) {
    return BADXMLFILE;
  }

  /*
   * treat filler
   */
  filler = mxmlElementGetAttr(current, "filler");
  if (filler == NULL) {
    return NOFILLER;
  }
  if (strcmp(filler, "tof") == 0) {
    pHM->histo_type = FILLERTOF;
  } else if (strcmp(filler, "psd") == 0) {
    pHM->histo_type = FILLERPSD;
  } else if (strcmp(filler, "dig") == 0) {
    pHM->histo_type = FILLERDIG;
  } else if (strcmp(filler, "tofmap") == 0) {
    pHM->histo_type = FILLERTOFMAP;
  } else if (strcmp(filler, "hrpt") == 0) {
    pHM->histo_type = FILLERHRPT;
  } else if (strcmp(filler, "sans2") == 0) {
    pHM->histo_type = FILLERSANS2;
  } else if ((strcmp(filler, "rita") == 0) || (strcmp(filler, "rita2") == 0)) {
    pHM->histo_type = FILLERRITA2;
  } else if (strcmp(filler, "camea") == 0) {
    pHM->histo_type = FILLERCAMEA;
  } else {
    return FILLERUNKNOWN;
  }

  pHM->bank_mapping_array.ptr = 0;
  attribute = mxmlElementGetAttr(current, "bankmaparray");
  if (attribute) {
    ardesc = (array_descr_type *)malloc(sizeof(array_descr_type));
    if (ardesc == NULL) {
      return NOMEMORY;
    }
    memset(ardesc, 0, sizeof(array_descr_type));

    status = loadDataArray(current, ardesc, attribute);
    if (status != 1) {
      return status;
    }
    length = ardesc->dim[0];
    for (i = 1; i < ardesc->rank; i++) {
      length *= ardesc->dim[i];
    }

    if (length > 0) {
      pHM->bank_mapping_array.ptr = ardesc;
    }
  }

  attribute = mxmlElementGetAttr(current, "increment");
  if (attribute) {
    pHM->increment = atoi(attribute);
  } else {
    pHM->increment = 1;
  }

  // set default values for hdr_daq_mask and hdr_daq_active
  // so that daq is not active, this forces correct values
  // in configuration file
  ui = 0;
  attribute = mxmlElementGetAttr(current, "hdr_daq_mask");
  if (attribute) {
    if (sscanf(attribute, "%x", &ui) != 1) {
      return XMLPARSEERROR;
    }
  }
  setControlVar(CFG_SRV_HDR_DAQ_MASK, ui);

  ui = 0xffffffff;
  attribute = mxmlElementGetAttr(current, "hdr_daq_active");
  if (attribute) {
    if (sscanf(attribute, "%x", &ui) != 1) {
      return XMLPARSEERROR;
    }
  }
  setControlVar(CFG_SRV_HDR_DAQ_ACTIVE, ui);

  /*
   * count banks
   */
  count = current;
  while ((count = mxmlFindElement(count, current, "bank", NULL, NULL,
                                  MXML_DESCEND)) != NULL) {
    nbank++;
  }
  if (nbank < 1) {
    return NOBANKS;
  }

  /*
   * allocate bank structures
   */
  //  pHM->length = nbank*3 + 1;                                /*
  //  bankoffset,rank,dataoffset per bank + ID*/
  pHM->bank_descr.ptr =
      (bank_descr_type *)malloc(nbank * sizeof(bank_descr_type));
  if (pHM->bank_descr.ptr == NULL) {
    return NOMEMORY;
  }
  for (i = 0; i < nbank; i++) {
    pHM->bank_descr.ptr[i].axis_descr.ptr = NULL;
  }
  pHM->nBank = nbank;

  /*
   * parse banks
   */
  count = current;
  for (i = 0; i < nbank; i++) {
    count = mxmlFindElement(count, current, "bank", NULL, NULL, MXML_DESCEND);
    status = parseBank(pHM, i, count);
    if (status < 0) {
      return status;
    }
  }
  return 1;
}

/*******************************************************************************/

dataShmOffs_type cp_array(volatile array_descr_type *ardesc) {
  int dataLength, i;
  int *iPtr;
  dataShmOffs_type axisData_Offs;

  if (!ardesc)
    return 0;

  // calculate length
  dataLength = ardesc->dim[0];
  for (i = 1; i < ardesc->rank; i++) {
    dataLength *= ardesc->dim[i];
  }

  axisData_Offs = dataShmAlloc((dataLength + 1 + ardesc->rank) * sizeof(int));
  if (axisData_Offs) {
    iPtr = (int *)dataShmOffsToPtr(axisData_Offs);

    iPtr[0] = ardesc->rank;
    for (i = 0; i < ardesc->rank; i++) {
      iPtr[i + 1] = ardesc->dim[i];
    }
    memcpy(iPtr + 1 + ardesc->rank, ardesc->data, dataLength * sizeof(int));
  }
  return axisData_Offs;
}

/*******************************************************************************/

dataShmOffs_type cp_tofmap_array(volatile array_descr_type *ardesc) {
  int dataLength, i, j;
  int *iPtr;
  dataShmOffs_type axisData_Offs;
  int tube, val, bin, inc, srcidx, dstidx;

  if (!ardesc)
    return 0;

  if (ardesc->rank == 2) {
    // array already in compressed form
    return cp_array(ardesc);
  }

  if (ardesc->rank != 3) {
    // array already in compressed form
    printf("ERROR: wrong rank in mapping tofmap array (must be 2 or 3)\n");
    return 0;
  }

  if (ardesc->dim[2] != 4) {
    // array already in compressed form
    printf("ERROR: length of 3. dimension must be 4\n");
    return 0;
  }

  // calculate length
  dataLength = ardesc->dim[0] * ardesc->dim[1];

  axisData_Offs = dataShmAlloc((dataLength + 3) * sizeof(int));
  if (axisData_Offs) {

    iPtr = (int *)dataShmOffsToPtr(axisData_Offs);

    iPtr[0] = 2;              // new rank is 2
    iPtr[1] = ardesc->dim[0]; // number of tubes
    iPtr[2] = ardesc->dim[1]; // position values of one tube.

    for (i = 0; i < ardesc->dim[0]; i++) {
      for (j = 0; j < ardesc->dim[1]; j++) {
        srcidx = (i * ardesc->dim[1] * ardesc->dim[2]) + (j * ardesc->dim[2]);
        tube = ardesc->data[srcidx];
        val = ardesc->data[srcidx + 1];
        bin = ardesc->data[srcidx + 2];
        inc = ardesc->data[srcidx + 3];

        dstidx = tube * ardesc->dim[1] + val;
        if (dstidx < dataLength) {
          iPtr[3 + dstidx] = (bin << 16) | (inc & 0xffff);
        } else {
          printf("ERROR: wrong index in tofmap array\n");
        }
      }
    }
  }

  return axisData_Offs;
}

/*******************************************************************************/

dataShmOffs_type cp_bankmap_array(volatile array_descr_type *ardesc) {
  int dataLength, i;
  int *iPtr;
  dataShmOffs_type axisData_Offs;
  int channel, bank, bankch, srcidx;

  if (!ardesc)
    return 0;

  if (ardesc->rank == 1) {
    // array already in compressed form
    return cp_array(ardesc);
  }

  if (ardesc->rank != 2) {
    // array already in compressed form
    printf("ERROR: rank of bank mapping array must be 1 or 2\n");
    return 0;
  }

  if (ardesc->dim[1] != 3) {
    // array already in compressed form
    printf("ERROR: length of 2. dimension must be 3\n");
    return 0;
  }

  // calculate length
  dataLength = ardesc->dim[0];

  axisData_Offs = dataShmAlloc((dataLength + 2) * sizeof(int));
  if (axisData_Offs) {

    iPtr = (int *)dataShmOffsToPtr(axisData_Offs);

    iPtr[0] = 1;              // new rank is 2
    iPtr[1] = ardesc->dim[0]; // number of channels

    for (i = 0; i < ardesc->dim[0]; i++) {

      srcidx = i * ardesc->dim[1];
      channel = ardesc->data[srcidx];
      bank = ardesc->data[srcidx + 1];
      bankch = ardesc->data[srcidx + 2];

      if (channel < dataLength) {
        iPtr[2 + channel] = (bank << 16) | (bankch & 0xffff);
      } else {
        printf("ERROR: wrong index in tofmap array\n");
      }
    }
  }

  return axisData_Offs;
}

/*******************************************************************************
 *
 * FUNCTION
 *   configureHisto
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int configureHisto(volatile histo_descr_type *histo_descr_ptr,
                   histo_descr_type *hmconfig) {
  int bank, axis;
  dataShmOffs_type axisData_Offs;
  volatile bankOffs_type *bankOffs_ptr;
  volatile bank_descr_type *bank_descr_ptr;
  volatile axis_descr_type *axis_descr_ptr;
  volatile array_descr_type *ardesc;

  histo_descr_ptr->bank_descr.offs =
      dataShmAlloc(hmconfig->nBank * sizeof(bankOffs_type));
  if (!histo_descr_ptr->bank_descr.offs)
    return NOSHMMEMORY;

  if (hmconfig->bank_mapping_array.ptr) {
    histo_descr_ptr->bank_mapping_array.offs =
        cp_bankmap_array(hmconfig->bank_mapping_array.ptr);
    if (!histo_descr_ptr->bank_mapping_array.offs)
      return CPARRAYERROR;
  } else {
    histo_descr_ptr->bank_mapping_array.offs = 0;
  }

  histo_descr_ptr->increment = hmconfig->increment;

  bankOffs_ptr = (volatile bankOffs_type *)dataShmOffsToPtr(
      histo_descr_ptr->bank_descr.offs);

  for (bank = 0; bank < hmconfig->nBank; bank++) {
    bankOffs_ptr[bank] = dataShmAlloc(sizeof(bank_descr_type));
    if (!bankOffs_ptr[bank])
      return NOSHMMEMORY;

    bank_descr_ptr =
        (volatile bank_descr_type *)dataShmOffsToPtr(bankOffs_ptr[bank]);
    bank_descr_ptr->rank = hmconfig->bank_descr.ptr[bank].rank;
    bank_descr_ptr->axis_descr.offs = dataShmAlloc(
        (hmconfig->bank_descr.ptr[bank].rank) * sizeof(axis_descr_type));
    if (!bank_descr_ptr->axis_descr.offs)
      return NOSHMMEMORY;

    bank_descr_ptr->bank_size = sizeof(uint32);
    axis_descr_ptr = (volatile axis_descr_type *)dataShmOffsToPtr(
        bank_descr_ptr->axis_descr.offs);

    /**
     * write axis descriptions heads
     */

    for (axis = 0; axis < hmconfig->bank_descr.ptr[bank].rank; axis++) {
      axis_descr_ptr[axis].length =
          hmconfig->bank_descr.ptr[bank].axis_descr.ptr[axis].length;
      axis_descr_ptr[axis].type =
          hmconfig->bank_descr.ptr[bank].axis_descr.ptr[axis].type;
      bank_descr_ptr->bank_size *= axis_descr_ptr[axis].length;

      switch (hmconfig->bank_descr.ptr[bank].axis_descr.ptr[axis].type) {
      case AXDIRECT:
        axis_descr_ptr[axis].axis_data.offs = 0;
        break;

      case AXCALC:
        axisData_Offs = dataShmAlloc(4 * sizeof(int));
        if (!axisData_Offs)
          return NOSHMMEMORY;
        memcpy(
            (void *)dataShmOffsToPtr(axisData_Offs),
            hmconfig->bank_descr.ptr[bank].axis_descr.ptr[axis].axis_data.ptr,
            4 * sizeof(int));
        axis_descr_ptr[axis].axis_data.offs = axisData_Offs;
        break;

      case AXBOUNDARY:
        axis_descr_ptr[axis].threshold =
            hmconfig->bank_descr.ptr[bank].axis_descr.ptr[axis].threshold;
        axis_descr_ptr[axis].offset =
            hmconfig->bank_descr.ptr[bank].axis_descr.ptr[axis].offset;
        ardesc = (array_descr_type *)hmconfig->bank_descr.ptr[bank]
                     .axis_descr.ptr[axis]
                     .axis_data.ptr;
        axis_descr_ptr[axis].axis_data.offs = cp_array(ardesc);
        if (!axis_descr_ptr[axis].axis_data.offs)
          return CPARRAYERROR;

        break;

      case AXLOOKUP:
        ardesc = (array_descr_type *)hmconfig->bank_descr.ptr[bank]
                     .axis_descr.ptr[axis]
                     .axis_data.ptr;

        switch (hmconfig->histo_type) {
        case FILLERTOFMAP:
          axis_descr_ptr[axis].axis_data.offs = cp_tofmap_array(ardesc);
          if (!axis_descr_ptr[axis].axis_data.offs)
            return CPARRAYERROR;
          break;

        default:
          axis_descr_ptr[axis].axis_data.offs = cp_array(ardesc);
          if (!axis_descr_ptr[axis].axis_data.offs)
            return CPARRAYERROR;
          break;
        }
        break;

        //  \author Michele Brambilla <michele.brambilla@psi.ch>
        //  \date Tue Apr 11 15:17:45 2017
      case AXMAP:
        ardesc = (array_descr_type *)hmconfig->bank_descr.ptr[bank]
                     .axis_descr.ptr[axis]
                     .axis_data.ptr;
        axis_descr_ptr[axis].axis_data.offs = cp_array(ardesc);
        if (!axis_descr_ptr[axis].axis_data.offs)
          return CPARRAYERROR;
        break;
      }
    }
  }

  // allocate Bank Data Memory after Bank Configuration Memory

  histo_descr_ptr->histo_mem_size = 0;
  for (bank = 0; bank < hmconfig->nBank; bank++) {
    bank_descr_ptr = getBankDescription(bank);
    bank_descr_ptr->bank_data_offs = dataShmAlloc(bank_descr_ptr->bank_size);
    if (!bank_descr_ptr->bank_data_offs)
      return NOSHMMEMORY;
    histo_descr_ptr->histo_mem_size += bank_descr_ptr->bank_size;
  }

  // set histo memory to first bank
  bank_descr_ptr = getBankDescription(0);
  histo_descr_ptr->histo_mem_offs = bank_descr_ptr->bank_data_offs;

  return 0;
}

/*******************************************************************************
 *
 * FUNCTION
 *   startConfiguration
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

static int startConfiguration(void) {
  int val, status;
  struct timespec req, rem;
  time_t starttime;

  status = setControlVar(CFG_SRV_DO_CFG_CMD, 0);
  if (status < 0) {
    return SYSTEMERROR;
  }
  req.tv_sec = 0;
  req.tv_nsec = 100;
  starttime = time(NULL);
  while (1) {
    nanosleep(&req, &rem);
    status = getControlVar(CFG_FIL_DO_CFG_ACK, &val);
    if (status < 0) {
      return SYSTEMERROR;
    }
    if (val == 0) {
      break;
    }
    if (time(NULL) > starttime + 5) {
      return SINQHM_ERR_FILLERTIMEOUT;
    }
  }
  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   finishConfiguration
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

static int finishConfiguration(void) {
  int val, status;
  struct timespec req, rem;
  time_t starttime;

  status = setControlVar(CFG_SRV_DO_CFG_CMD, 1);
  if (status < 0) {
    return SYSTEMERROR;
  }
  req.tv_sec = 0;
  req.tv_nsec = 100;
  starttime = time(NULL);
  while (1) {
    nanosleep(&req, &rem);
    status = getControlVar(CFG_FIL_DO_CFG_ACK, &val);
    if (status < 0) {
      return SYSTEMERROR;
    }
    if (val == 1) {
      break;
    }
    if (time(NULL) > starttime + 5) {
      return SINQHM_ERR_FILLERTIMEOUT;
    }
  }
  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   configureDataSegment
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int configureDataSegment(histo_descr_type *hmconfig) {
  int status;
  volatile histo_descr_type *histo_descr_ptr;

  status = startConfiguration();
  if (status < 0)
    return status;

  histo_descr_ptr = getShmHistoPtr();

  if (!histo_descr_ptr) {
    return SINQHM_ERR_NOTCONFIGURED;
  }

  // clear static configuration section of Histo SHM
  memset((void *)histo_descr_ptr, 0,
         (HM_CFG_DYNAMIC_MEM_START * sizeof(uint32)));

  /* free any previous allocated data memory */
  dataShmFreeAll();

  histo_descr_ptr->id = DATASHM_ID;
  histo_descr_ptr->version = DATASHM_VERSION;
  histo_descr_ptr->server_valid = 0;
  histo_descr_ptr->filler_valid = 0;
  histo_descr_ptr->cfg_mem_size = hst_size;
  histo_descr_ptr->histo_mem_size =
      0; /* actual size is set in configureHisto */
  histo_descr_ptr->histo_mem_offs = 0;
  histo_descr_ptr->histo_type = hmconfig->histo_type;
  histo_descr_ptr->nBank = hmconfig->nBank;
  histo_descr_ptr->bank_descr.offs = 0;

  histo_descr_ptr->rawdata_offs = 0;
  histo_descr_ptr->rawdata_size = 0;
  histo_descr_ptr->rawdata_stored = 0;
  histo_descr_ptr->rawdata_missed = 0;

  status = configureHisto(histo_descr_ptr, hmconfig);
  if (status < 0)
    return status;

  histo_descr_ptr->server_valid = DATASHM_CFG_SRV_VALID;

  status = finishConfiguration();
  if (status < 0)
    return status;

  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   deleteConfig
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

void deleteConfig(histo_descr_type *hmconfig) {
  int i, j;
  array_descr_type *ardesc;

  for (i = 0; i < hmconfig->nBank; i++) {
    if (hmconfig->bank_descr.ptr[i].axis_descr.ptr != NULL) {
      for (j = 0; j < hmconfig->bank_descr.ptr[i].rank; j++) {
        if (hmconfig->bank_descr.ptr[i].axis_descr.ptr[j].axis_data.ptr !=
            NULL) {
          if (hmconfig->bank_descr.ptr[i].axis_descr.ptr[j].type ==
                  AXBOUNDARY ||
              hmconfig->bank_descr.ptr[i].axis_descr.ptr[j].type == AXLOOKUP ||
              //  \author Michele Brambilla <michele.brambilla@psi.ch>
              //  \date Tue Apr 11 15:18:20 2017
              hmconfig->bank_descr.ptr[i].axis_descr.ptr[j].type == AXMAP) {
            ardesc = (array_descr_type *)hmconfig->bank_descr.ptr[i]
                         .axis_descr.ptr[j]
                         .axis_data.ptr;
            if (ardesc != NULL) {
              if (ardesc->dim != NULL) {
                free(ardesc->dim);
              }
              if (ardesc->data != NULL) {
                free(ardesc->data);
              }
            }
          }
          free(hmconfig->bank_descr.ptr[i].axis_descr.ptr[j].axis_data.ptr);
        }
      }
      free(hmconfig->bank_descr.ptr[i].axis_descr.ptr);
    }
  }
  free(hmconfig->bank_descr.ptr);
}

/*******************************************************************************
 *
 * FUNCTION
 *   configureHistogramMemoryFromFile
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int configureHistogramMemoryFromFile(char *filename, int test_only) {
  mxml_node_t *root = NULL;
  histo_descr_type hmconfig;
  FILE *fp = NULL;
  int status;

  fp = fopen(filename, "r");

  if (fp == NULL) {
    return BADFILE;
  }
  root = mxmlLoadFile(NULL, fp, MXML_INTEGER_CALLBACK);
  fclose(fp);

  if (root == NULL) {
    return XMLPARSEERROR;
  }
  status = parseConfiguration(&hmconfig, root);
  mxmlDelete(root);
  if (status < 0) {
    deleteConfig(&hmconfig);
    return status;
  }

  if (!test_only) {
    status = configureDataSegment(&hmconfig);
    if (status < 0) {
      deleteConfig(&hmconfig);
      return status;
    }
  }

  deleteConfig(&hmconfig);
  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   configureHistogramMemory
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int configureHistogramMemory(char *configBuffer, int test_only) {
  mxml_node_t *root = NULL;
  histo_descr_type hmconfig;
  int status;

  root = mxmlLoadString(NULL, configBuffer, MXML_INTEGER_CALLBACK);

  if (root == NULL) {
    return XMLPARSEERROR;
  }
  status = parseConfiguration(&hmconfig, root);
  mxmlDelete(root);
  if (status < 0) {
    deleteConfig(&hmconfig);
    return status;
  }

  if (!test_only) {
    status = configureDataSegment(&hmconfig);
    if (status < 0) {
      deleteConfig(&hmconfig);
      return status;
    }
  }

  deleteConfig(&hmconfig);
  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   startDAQ
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int startDAQ(void) {
  int val, status;
  struct timespec req, rem;
  time_t starttime;

  status = setControlVar(CFG_SRV_DAQ_PAUSE_CMD, 0);
  if (status < 0) {
    return SYSTEMERROR;
  }

  status = setControlVar(CFG_SRV_DO_DAQ_CMD, 1);
  if (status < 0) {
    return SYSTEMERROR;
  }
  req.tv_sec = 0;
  req.tv_nsec = 100;
  starttime = time(NULL);
  while (1) {
    nanosleep(&req, &rem);
    status = getControlVar(CFG_FIL_DO_DAQ_ACK, &val);
    if (status < 0) {
      return SYSTEMERROR;
    }
    if (val == 1) {
      break;
    }
    if (time(NULL) > starttime + 5) {
      return SINQHM_ERR_FILLERTIMEOUT;
    }
  }
  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   stopDAQ
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int stopDAQ(void) {
  int val, status;
  struct timespec req, rem;
  time_t starttime;

  status = setControlVar(CFG_SRV_DO_DAQ_CMD, 0);
  if (status < 0) {
    return SYSTEMERROR;
  }
  req.tv_sec = 0;
  req.tv_nsec = 100;
  starttime = time(NULL);
  while (1) {
    nanosleep(&req, &rem);
    status = getControlVar(CFG_FIL_DO_DAQ_ACK, &val);
    if (status < 0) {
      return SYSTEMERROR;
    }
    if (val == 0) {
      break;
    }
    if (time(NULL) > starttime + 5) {
      return SINQHM_ERR_FILLERTIMEOUT;
    }
  }
  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   pauseDAQ
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int pauseDAQ(void) {
  int status;

  status = setControlVar(CFG_SRV_DAQ_PAUSE_CMD, 1);
  if (status < 0) {
    return SYSTEMERROR;
  }
  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   dataErrorToText
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

void dataErrorToText(int errorCode, char *buffer, int buflen) {
  switch (errorCode) {
  case XMLPARSEERROR:
    strncpy(buffer, "Malformed XML file", buflen);
    break;
  case SINQHM_ERR_NOTCONFIGURED:
    strncpy(buffer, "The data shared memory is not yet configured", buflen);
    break;
  case BADXMLFILE:
    strncpy(buffer, "No Sinqhm XML file", buflen);
    break;
  case NOFILLER:
    strncpy(buffer, "No filler attribute to sinqhm element", buflen);
    break;
  case FILLERUNKNOWN:
    strncpy(buffer, "Filler type to sinqhm element not recognized", buflen);
    break;
  case NOBANKS:
    strncpy(buffer, "No detector banks defined", buflen);
    break;
  case NOMEMORY:
    strncpy(buffer, "Out of memory allocating data structures", buflen);
    break;
  case NOAXIS:
    strncpy(buffer, "No axis elements found", buflen);
    break;
  case NOAXISLENGTH:
    strncpy(buffer, "Length attribute for axis missing", buflen);
    break;
  case NOAXISMAPPING:
    strncpy(buffer, "Mapping attribute for axis missing", buflen);
    break;
  case NOOFFSET:
    strncpy(buffer, "Offset attribute to calculate axis missing", buflen);
    break;
  case NODIVISOR:
    strncpy(buffer, "Divisor attribute to calculate axis missing", buflen);
    break;
  case BADMAPPING:
    strncpy(buffer, "Invalid mapping type requested for axis", buflen);
    break;
  case BADFILE:
    strncpy(buffer, "Failed to open configuration file", buflen);
    break;
  case AXARRAYTOSHORT:
    strncpy(buffer, "Axis array to short!", buflen);
    break;
  case SINQHM_ERR_NOSUCHBANK:
    strncpy(buffer, "No bank matching the given index", buflen);
    break;
  case SINQHM_ERR_BADCONFIG:
    strncpy(buffer, "Histogram memory badly or not configured", buflen);
    break;
  case SINQHM_ERR_FILLERTIMEOUT:
    strncpy(buffer, "The filler process did not respond to a command", buflen);
    break;
  case SYSTEMERROR:
    strncpy(buffer,
            "System error: symptom: could not retrive data from control SHM",
            buflen);
    break;
  case MAPMUSTBESECOND:
    strncpy(buffer, "A mapped axis must be at least second", buflen);
    break;
  case NOARRAYRANK:
    strncpy(buffer, "Array element has no rank attribute", buflen);
    break;
  case NOARRAYDIM:
    strncpy(buffer, "Array element has no dim attribute", buflen);
    break;
  case NOTENOUGHDIM:
    strncpy(buffer, "Not enough dimensions specified for array", buflen);
    break;
  case ARRAYSHORT:
    strncpy(buffer, "Axis array shorter then calculated from dims", buflen);
    break;
  case ARRAYMISSING:
    strncpy(buffer,
            "No array for mapping specified for axis of type boundary or "
            "lookuptable",
            buflen);
    break;
  case NOSHMMEMORY:
    strncpy(buffer, "Not enough shared memory", buflen);
    break;
  case CPARRAYERROR:
    strncpy(buffer, "Error copying array to shared memory", buflen);
    break;
  default:
    snprintf(buffer, buflen, "Unknown error code: %d", errorCode);
    break;
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   resetBuff
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int resetBuff(void) {
  int val, status;
  struct timespec req, rem;
  time_t starttime = 0;

  status = getControlVar(CFG_FIL_RST_PRINT_BUFF_ACK, &val);
  if (status < 0)
    return SYSTEMERROR;

  if (val) {
    status = setControlVar(CFG_SRV_RST_PRINT_BUFF_CMD, 0);
    while (1) {
      nanosleep(&req, &rem);
      status = getControlVar(CFG_FIL_RST_PRINT_BUFF_ACK, &val);
      if (status < 0) {
        return SYSTEMERROR;
      }
      if (val == 0) {
        break;
      }
      if (time(NULL) > starttime + 5) {
        return SINQHM_ERR_FILLERTIMEOUT;
      }
    }
  }

  status = setControlVar(CFG_SRV_RST_PRINT_BUFF_CMD, 1);
  if (status < 0) {
    return SYSTEMERROR;
  }
  req.tv_sec = 0;
  req.tv_nsec = 100;
  starttime = time(NULL);
  while (1) {
    nanosleep(&req, &rem);
    status = getControlVar(CFG_FIL_RST_PRINT_BUFF_ACK, &val);
    if (status < 0) {
      return SYSTEMERROR;
    }
    if (val == 1) {
      break;
    }
    if (time(NULL) > starttime + 5) {
      return SINQHM_ERR_FILLERTIMEOUT;
    }
  }

  status = setControlVar(CFG_SRV_RST_PRINT_BUFF_CMD, 0);
  return 1;
}
/******************************************************************************/
static pNXDS getHMDataAsDataset(int bankno, char *error, int errLen) {
  pNXDS result = NULL;
  volatile bank_descr_type *bank = NULL;
  volatile axis_descr_type *axis = NULL;

  int i;

  bank = getBankDescription(bankno);
  if (bank == NULL) {
    strncpy(error, "ERROR: invalid bank requested", errLen);
    return NULL;
  }

  /*
    initialize a NXDS datastructure to contain the current HM data
  */
  result = (pNXDS)malloc(sizeof(NXDS));
  if (result == NULL) {
    strncpy(error, "ERROR: out of memory in HM", errLen);
    return NULL;
  }
  memset(result, 0, sizeof(NXDS));
  result->type = NX_UINT32;
  result->magic = MAGIC;
  result->rank = bank->rank;
  result->dim = (int *)malloc(result->rank * sizeof(int));
  if (result->dim == NULL) {
    free(result);
    strncpy(error, "ERROR: out of memory in HM", errLen);
    return NULL;
  }
  for (i = 0; i < result->rank; i++) {
    axis = getAxisDescription(bankno, i);
    if (axis == NULL) {
      strncpy(error, "ERROR: failed to get axis in getHMdataAsDataset", errLen);
      free(result->dim);
      free(result);
      return NULL;
    }
    result->dim[i] = axis->length;
  }
  result->u.iPtr = getBankData(bankno);
  if (result->u.iPtr == NULL) {
    strncpy(error, "ERROR: failed to get HM data in getHMdataAsDataset",
            errLen);
    free(result->dim);
    free(result);
    return NULL;
  }
  return result;
}
/********************************************************************************/
static pNXDS subSampleCommand(pNXDS source, char *command, char *error,
                              int errLen) {
  int startDim[NX_MAXRANK], endDim[NX_MAXRANK];
  int dim = 0, start = 0, i;
  char *pPtr = NULL, token[80];

  pPtr = stptok(command, token, 79, ":\0");
  while ((pPtr = stptok(pPtr, token, 79, ":\0")) != NULL) {
    if (start == 0) {
      startDim[dim] = atoi(token);
      start = 1;
    } else {
      endDim[dim] = atoi(token);
      start = 0;
      dim++;
    }
  }

  if (dim < source->rank - 1) {
    strncpy(error, "ERROR: Not enough border values specified for subsampling",
            errLen);
    return NULL;
  }
  for (i = 0; i < source->rank; i++) {
    if (startDim[i] < 0 || startDim[i] >= source->dim[i]) {
      snprintf(error, errLen, "ERROR: invalid start value %d for dimension %d",
               startDim[1], i);
      return NULL;
    }
    if (endDim[i] < startDim[i] || endDim[i] >= source->dim[i]) {
      snprintf(error, errLen, "ERROR: invalid end value %d for dimension %d",
               endDim[1], i);
      return NULL;
    }
  }

  return cutNXDataset(source, startDim, endDim);
}
/*-----------------------------------------------------------------------------*/
static pNXDS sumCommand(pNXDS source, char *command, char *error, int errlen) {
  int dimNo = -1, start = -1, end = -1;
  char *pPtr = NULL;
  char token[80];

  pPtr = stptok(command, token, 79, ":\0");
  pPtr = stptok(pPtr, token, 79, ":\0");
  if (pPtr != NULL) {
    dimNo = atoi(token);
  }
  pPtr = stptok(pPtr, token, 79, ":\0");
  if (pPtr != NULL) {
    start = atoi(token);
  }
  pPtr = stptok(pPtr, token, 79, ":\0");
  if (pPtr != NULL) {
    end = atoi(token);
  }
  if (dimNo < 0 || dimNo > source->rank - 1) {
    snprintf(error, errlen, "ERROR: invalid dimension %d requestd to sum",
             dimNo);
    return NULL;
  }
  if (end < 0 || end > source->dim[dimNo] || start < 0 || start > end) {
    snprintf(error, errlen, "ERROR: invalid summing limits %d to %d requested",
             start, end);
    return NULL;
  }

  return sumNXDataset(source, dimNo, start, end);
}
/*------------------------------------------------------------------------------*/
pNXDS processData(int bankno, char *command, char *error, int errLen) {
  pNXDS source = NULL, start = NULL;
  pNXDS result = NULL;
  char *pPtr = NULL;
  char subCommand[132];

  start = getHMDataAsDataset(bankno, error, errLen);
  if (start == NULL) {
    return NULL;
  }

  source = start;
  pPtr = command;
  while ((pPtr = stptok(pPtr, subCommand, 131, ";\0\r\n")) != NULL) {
    if (strstr(subCommand, "sample") != NULL) {
      result = subSampleCommand(source, subCommand, error, errLen);
    } else if (strstr(subCommand, "sum") != NULL) {
      result = sumCommand(source, subCommand, error, errLen);
    } else {
      strncpy(error, "ERROR: invalid subcommand to process requested", errLen);
      return NULL;
    }
    if (result == NULL) {
      return NULL;
    }
    if (source != start) {
      dropNXDataset(source);
    }
    source = result;
  }
  return result;
}
