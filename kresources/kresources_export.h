/*
  This file is part of the kresources library.
  Copyright (c) 2006 Allen Winter <winter@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef KRESOURCES_EXPORT_H
#define KRESOURCES_EXPORT_H

#include <kdemacros.h>

#ifndef KRESOURCES_EXPORT
# if defined(KDEPIM_STATIC_LIBS)
   /* No export/import for static libraries */
#  define KRESOURCES_EXPORT
# elif defined(MAKE_KRESOURCES_LIB)
   /* We are building this library */
#  define KRESOURCES_EXPORT KDE_EXPORT
# else
   /* We are using this library */
#  define KRESOURCES_EXPORT KDE_IMPORT
# endif
#endif

#ifndef KCM_KRESOURCES_EXPORT
# if defined(KDEPIM_STATIC_LIBS)
   /* No export/import for static libraries */
#  define KCM_KRESOURCES_EXPORT
# elif defined(MAKE_KCM_KRESOURCES_LIB)
   /* We are building this library */
#  define KCM_KRESOURCES_EXPORT KDE_EXPORT
# else
   /* We are using this library */
#  define KCM_KRESOURCES_EXPORT KDE_IMPORT
# endif
#endif

# ifndef KRESOURCES_EXPORT_DEPRECATED
#  if !defined( WANT_DEPRECATED_KRESOURCE_API )
#    define KRESOURCES_EXPORT_DEPRECATED KDE_DEPRECATED KRESOURCES_EXPORT
#  else
#    define KRESOURCES_EXPORT_DEPRECATED KRESOURCES_EXPORT
#  endif
# endif
# ifndef KCM_KRESOURCES_EXPORT_DEPRECATED
#  if !defined( WANT_DEPRECATED_KRESOURCE_API )
#    define KCM_KRESOURCES_EXPORT_DEPRECATED KDE_DEPRECATED KCM_KRESOURCES_EXPORT
#  else
#    define KCM_KRESOURCES_EXPORT_DEPRECATED KCM_KRESOURCES_EXPORT
#  endif
# endif

#endif
