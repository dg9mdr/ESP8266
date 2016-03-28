 //
// ************************************************************************
// A rework and enhancement to ESPs as sensor nodes
// (C) 2016 Dirk Schanz aka dreamshader
//
// Functional description:
// step by step add protocols, sensors, actors and functionalities
// to ESP modules. In opposition to the last solution try to hold
// all features modular.
//
// ************************************************************************
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ************************************************************************
//
//-------- History -------------------------------------------------
//
// 1st version: 03/28/16
//         Web forms for node administration, complete rework on EEPROM
//         layout.
// update:
//
//
// ************************************************************************
// first of all include configuration definitions
// ************************************************************************
//

String pageContent;

#define SERVER_METHOD_GET       1
#define SERVER_METHOD_POST      2

#define ARGS_ADMIN_PAGE         2

