/*
 * © Copyright IBM Corporation 2020
 */

/*
 * Copyright © 2001-2020 Python Software Foundation; All Rights Reserved
 */

#define PY_SSIZE_T_CLEAN
#define _OPEN_SYS_FILE_EXT 1
#include <Python.h>
#include <object.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#define BINARY_CCSID 65535
#define UNTAG_CCSID 0
#define ISO8859_CCSID 819
#define IBM1047_CCSID 1047

static int __vm__ = 0;

const static int iso8859 = AUDTREADFAIL  | AUDTREADSUCC  |
                           AUDTWRITEFAIL | AUDTWRITESUCC |
                           AUDTEXECFAIL;                  

const static int ibm1047 = AUDTREADFAIL  |                
                           AUDTWRITEFAIL |                
                           AUDTEXECFAIL;                  

const static int binary  = AUDTREADFAIL  |                
                           AUDTWRITEFAIL |                
                           AUDTEXECFAIL  | AUDTEXECSUCC;  

const static int mixAsc  = AUDTREADFAIL  | AUDTREADSUCC  |
                           AUDTWRITEFAIL |                
                           AUDTEXECFAIL  | AUDTEXECSUCC;  

const static int mixEbc  = AUDTREADFAIL  |               
                           AUDTWRITEFAIL | AUDTWRITESUCC |
                           AUDTEXECFAIL  | AUDTEXECSUCC;  

static inline int
isebcdic(char c)
{
  if ((c >= 0x40) & (c < 0xff))
     return (1);
  else
     return (0);
}

static PyObject *
__setccsid(PyObject *self, PyObject *args, PyObject *kwargs, int ccsid) 
{
  char *path;
  int txtflag = 1;
  int res;
  attrib_t attr;

  memset(&attr, 0, sizeof(attr));
  attr.att_filetagchg = 1;

  if (ccsid == UNTAG_CCSID || ccsid == BINARY_CCSID) {
    if (!PyArg_ParseTuple(args, "s", &path))
      return NULL;
    attr.att_filetag.ft_txtflag = 0;
  } else {

    static char *keywords[] = {"filepath", "ccsid", "set_txtflag", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|ip", keywords, &path,
                                     &ccsid, &txtflag))
      return NULL;
    attr.att_filetag.ft_txtflag = txtflag;
  }

  attr.att_filetag.ft_ccsid = ccsid;
  res = __chattr(path, &attr, sizeof(attr));
  if (res < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *
__settxtflag(PyObject *self, PyObject *args, int txtflag) 
{
  char *path;
  int res;
  struct stat st;
  attrib_t attr;
  unsigned short ccsid;

  if (!PyArg_ParseTuple(args, "s", &path))
    return NULL;

  memset(&attr, 0, sizeof(attr));
  attr.att_filetagchg = 1;

  // get old ccsid
  res = stat(path, &st);
  if (res < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
  }
  
  if (__vm__) {
    if ((st.st_useraudit == iso8859) ||
        (st.st_useraudit == mixAsc))
      ccsid = ISO8859_CCSID;
    else if (st.st_useraudit == binary)
      ccsid = BINARY_CCSID;
    else
      ccsid = IBM1047_CCSID;
  } else {
    ccsid =  st.st_tag.ft_ccsid;
  }

  attr.att_filetag.ft_ccsid = ccsid;
  attr.att_filetag.ft_txtflag = txtflag;

  res = __chattr(path, &attr, sizeof(attr));
  if (res < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *
__set_apf_auth(PyObject *self, PyObject *args, int is_apf) 
{
  char *path;
  int res;
  attrib_t attr;

  if (!PyArg_ParseTuple(args, "s", &path))
    return NULL;

  memset(&attr, 0, sizeof(attr));
  attr.att_setgen = 1;
  attr.att_apfauthmask = 1;
  attr.att_apfauth = is_apf;

  res = __chattr(path, &attr, sizeof(attr));
  if (res < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *
_chtag_impl(PyObject *self, PyObject *args, PyObject *kwargs) 
{
  return __setccsid(self, args, kwargs, ISO8859_CCSID);
}

static PyObject *
_tag_binary_impl(PyObject *self, PyObject *args) 
{
  return __setccsid(self, args, NULL, BINARY_CCSID);
}

static PyObject *
_untag_impl(PyObject *self, PyObject *args) 
{
  return __setccsid(self, args, NULL, UNTAG_CCSID);
}

static PyObject *
_tag_text_impl(PyObject *self, PyObject *args) 
{
  return __settxtflag(self, args, 1);
}

static PyObject *
_tag_mixed_impl(PyObject *self, PyObject *args) 
{
  return __settxtflag(self, args, 0);
}

static PyObject *
_get_tag_impl(PyObject *self, PyObject *args) 
{
  struct stat st;
  char *path;
  int res;
  int txtflag;
  unsigned short ccsid;

  if (!PyArg_ParseTuple(args, "s", &path))
    return NULL;

  res = stat(path, &st);
  if (res < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
  }

  if (__vm__) {
    if (st.st_useraudit == iso8859) {
      ccsid = ISO8859_CCSID;
      txtflag = 1;
    } else if (st.st_useraudit == mixAsc) {
        ccsid = ISO8859_CCSID;
        txtflag = 0;
    } else if (st.st_useraudit == binary) {
        ccsid = BINARY_CCSID;
        txtflag = 0;
    } else if (st.st_useraudit == ibm1047) {
        ccsid = IBM1047_CCSID;
        txtflag = 1;
    } else if (st.st_useraudit == mixEbc) {
        ccsid = IBM1047_CCSID;
        txtflag = 0;
    }    
  } else {
    ccsid =  st.st_tag.ft_ccsid;
    txtflag = st.st_tag.ft_txtflag;
  }

  return Py_BuildValue("(HN)", ccsid, PyBool_FromLong((long)(txtflag)));
}

static PyObject *
_get_tagging_impl(PyObject *self, PyObject *args) 
{
  struct stat st;
  char *path;
  PyObject *obj;
  int res;
  unsigned short ccsid;

  if (!PyArg_ParseTuple(args, "O", &obj))
    return NULL;

  if (PyBytes_Check(obj)) {
    path = PyBytes_AsString(obj);
    int i, asc = 1, ebc = 1;
    for (i = 0; i < PyBytes_GET_SIZE(obj); i++) {
      if (!isascii(path[i]))
        asc = 0;
      if (!isebcdic(path[i]))
        ebc = 0;
    }
    if (asc) 
      ccsid = ISO8859_CCSID;
    else if (ebc)
      ccsid = IBM1047_CCSID;
    else 
      ccsid = BINARY_CCSID;
  } else if (!PyUnicode_Check(obj)) {
    return NULL;
  } else {
    path = (char *) PyUnicode_AsUTF8(obj);
    
    res = stat(path, &st);
    if (res < 0) {
      PyErr_SetFromErrno(PyExc_OSError);
      return NULL;
    }
  
    if (__vm__) {
      if (st.st_useraudit == iso8859) {
        ccsid = ISO8859_CCSID;
      } else if (st.st_useraudit == mixAsc) {
          ccsid = ISO8859_CCSID;
      } else if (st.st_useraudit == binary) {
          ccsid = BINARY_CCSID;
      } else if (st.st_useraudit == ibm1047) {
          ccsid = IBM1047_CCSID;
      } else if (st.st_useraudit == mixEbc) {
          ccsid = IBM1047_CCSID;
      }    
    } else {
      ccsid =  st.st_tag.ft_ccsid;
    }
  }
  return Py_BuildValue("h", ccsid);
}

static PyObject *
_get_tagging_f_impl(PyObject *self, PyObject *args) 
{
  return _get_tag_impl(self, args);
}  

static PyObject *
_enable_apf_auth_impl(PyObject *self, PyObject *args) 
{
  return __set_apf_auth(self, args, 1);
}

static PyObject *
_disable_apf_auth_impl(PyObject *self, PyObject *args) 
{
  return __set_apf_auth(self, args, 0);
}

static PyObject *
_set_tagging_impl(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *path;
  int txtflag = 1;
  int res;
  int ccsid;
  attrib_t attr;
  PyObject *obj;
  static char *keywords[] = {"filepath", "ccsid", NULL};

  memset(&attr, 0, sizeof(attr));

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi", keywords, &obj,
                                   &ccsid))
    return NULL;

  if (PyBytes_Check(obj))
    path = PyBytes_AsString(obj);
  else if (PyUnicode_Check(obj))
    path = (char *) PyUnicode_AsUTF8(obj);
  else
    return NULL;
    
  attr.att_filetagchg = 1;
  if ((ccsid == UNTAG_CCSID) || (ccsid == BINARY_CCSID))
    attr.att_filetag.ft_txtflag = 0;
  else
    attr.att_filetag.ft_txtflag = 1;
  attr.att_filetag.ft_ccsid = ccsid;

  res = __chattr(path, &attr, sizeof(attr));
  perror("__chattr");
  if (res < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *
_zos_realpath_impl(PyObject *self, PyObject *args) 
{
  char *path, 
       resolved[_XOPEN_PATH_MAX];

  if (!PyArg_ParseTuple(args, "s", &path))
    return NULL;
        
  if (realpath((const char *) path, (char *) &resolved) == NULL)
    return NULL;

  return Py_BuildValue("s", resolved);
}

PyDoc_STRVAR(
    chtag_doc,
    "chtag(filepath, ccsid=819, set_txtflag=True)\n"
    "--\n"
    "\n"
    "Chtag allows you to change information in a file tag, A file tag is "
    "composed\n"
    "composed of a numeric coded character set identifier(ccsid) and a text "
    "flag\n"
    "(set_txtflag) codeset: \n "
    "\t Represents the coded character set in which text data is\n"
    "\t encoded. The code set can be used for uniformly encoded text files or\n"
    "\t files that contain mixed text/binary data.\n"
    "set_txtflag: \n"
    "\t set_txtflag = True indicates the file has uniformly encoded text data\n"
    "\t set_txtflag = False indicates the file has non-uniformly encoded text "
    "data\n");

PyDoc_STRVAR(untag_doc, "untag(filepath)\n"
                        "--\n"
                        "\n"
                        "Removes any tagging information associated with the "
                        "file and sets the status\n"
                        "of the file to 'untagged'.");

PyDoc_STRVAR(get_tag_info_doc, "get_tag_info(filepath)\n"
                               "--\n"
                               "\n"
                               "Return a tuple (ccsid, set_txtflag) of file "
                               "tag information associated with the\n"
                               "file.");

PyDoc_STRVAR(tag_binary_doc, "tag_binary(filepath)\n"
                             "--\n"
                             "\n"
                             "Change the file tag to binary mode. Indicates "
                             "that the file contains only \n"
                             "binary (non-uniformly encoded) data.\n ");

PyDoc_STRVAR(
    tag_text_doc,
    "tag_text(filepath)\n"
    "--\n"
    "\n"
    "Change the file tag to text mode, Indicates that the specified file "
    "contains\n"
    "pure (uniformly encoded) text data.\n"
    "The existing ccsid that is associated with the file is retained.\n"
    "Note: the ccsid MUST be set before apply this function\n ");

PyDoc_STRVAR(
    tag_mixed_doc,
    "tag_mixed(filepath)\n"
    "--\n"
    "\n"
    "Change the file tag to mixed mode, Indicates that the file contains mixed "
    "text\n"
    "and binary data.\n"
    "The existing ccsid that is associated with the file is retained.\n"
    "Note: the ccsid MUST be set before apply this function\n");

PyDoc_STRVAR(enable_apf_auth_doc,
             "enable_apf(filepath)\n"
             "--\n"
             "\n"
             "Set APF-authorized attribute to an executable program file(load "
             "module), it\n"
             "behaves as if loaded from an APF-authorized library.\n"
             "For example, if this program is exec()ed at the job step level "
             "and the program\n"
             "is linked with the AC=1 attribute, the program will be executed "
             "as APF-authorized.\n"
             "Rule: To specify any of these attributes, the user must be the "
             "owner of the file\n"
             "or have superuser authority.\n");

PyDoc_STRVAR(disable_apf_auth_doc,
             "disable_apf(filepath)\n"
             "--\n"
             "\n"
             "Unset APF-authorized attribute to an executable program file,it "
             "behaves as \n"
             "remove file from an APF-authorized library.\n"
             "Rule: To specify any of these attributes, the user must be the "
             "owner of the\n"
             "file or have superuser authority.\n");

PyDoc_STRVAR(get_tagging_doc, "get_tagging(filepath)\n"
                              "--\n"
                              "\n"
                              "Return a ccsid of file "
                              "tag information associated with the\n"
                              "file.");

PyDoc_STRVAR(get_tagging_f_doc, "get_tagging_f_info(filepath)\n"
                                "--\n"
                                "\n"
                                "Return a tuple (ccsid, set_txtflag) of file "
                                "tag information associated with the\n"
                                "file.");

PyDoc_STRVAR(set_tagging_doc, "set_tagging(filepath, ccsid)\n"
                              "--\n"
                              "\n"
                              "Set the ccsid of file tag for the specified file.");

PyDoc_STRVAR(zos_realpath_doc, "zos_realpath(filepath)\n"
                               "--\n"
                               "\n"
                               "Return the real path of the z/OS file.");

static PyMethodDef ZosUtilMethods[] = {
    {"chtag", (PyCFunction)_chtag_impl, METH_VARARGS | METH_KEYWORDS,
     chtag_doc},
    {"untag", (PyCFunction)_untag_impl, METH_VARARGS, untag_doc},
    {"tag_binary", (PyCFunction)_tag_binary_impl, METH_VARARGS, tag_binary_doc},
    {"tag_mixed", (PyCFunction)_tag_mixed_impl, METH_VARARGS, tag_mixed_doc},
    {"tag_text", (PyCFunction)_tag_text_impl, METH_VARARGS, tag_text_doc},
    {"get_tag_info", (PyCFunction)_get_tag_impl, METH_VARARGS,
     get_tag_info_doc},
    {"disable_apf", (PyCFunction)_disable_apf_auth_impl, METH_VARARGS,
     disable_apf_auth_doc},
    {"enable_apf", (PyCFunction)_enable_apf_auth_impl, METH_VARARGS,
     enable_apf_auth_doc},
    {"get_tagging", (PyCFunction)_get_tagging_impl, METH_VARARGS | METH_KEYWORDS,
     get_tagging_doc},
    {"get_tagging_f", (PyCFunction)_get_tagging_f_impl, METH_VARARGS,
     get_tagging_f_doc},
    {"set_tagging", (PyCFunction)_set_tagging_impl, METH_VARARGS | METH_KEYWORDS,
     set_tagging_doc},
    {"zos_realpath", (PyCFunction)_zos_realpath_impl, METH_VARARGS,
     zos_realpath_doc},
    {NULL, NULL, 0, NULL}, // sentinel
};

static PyModuleDef zos_util = {
    PyModuleDef_HEAD_INIT,
    "zos.util",
    "This module provides a portable way of using Zos utility",
    -1,
    ZosUtilMethods,
};

PyMODINIT_FUNC PyInit_zos_util() {
  PyObject *module;
  struct utsname *buf = __alloca(512);  /* z/VM's utsname is much larger than z/OS's */

  module = PyModule_Create(&zos_util);
  if (module == NULL) {
    return NULL;
  }
  if (uname(buf) == 0) {
    if (strcmp(buf->sysname, "z/VM") == 0)
      __vm__ = 1;
  }
  return module;
}
