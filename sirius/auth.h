/**
 * @brief Authentication Helpers
 *
 * This library allows multiple login programs (login, sudo, glogin)
 * to share authentication code by providing a single palce to check
 * passwords against /etc/master.passwd and to set typical login vars.
 *
 * @copyright
 * This file is part of SiriusOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013-2018 K. Lange
 * Copyright (C) 2024 Gamma Microsystems
 */

#pragma once

#include <_cheader.h>
#include <unistd.h>

_Begin_C_Header

/**
 * sirius_auth_check_pass
 *
 * Returns the uid for the request user on success, -1 on failure.
 */
extern int sirius_auth_check_pass(char * user, char * pass);

/**
 * sirius_auth_set_vars
 *
 * Sets various environment variables (HOME, USER, SHELL, etc.)
 * for the current user.
 */
extern void sirius_auth_set_vars(void);

/**
 * Set supplementary groups from /etc/groups
 */
extern void sirius_auth_set_groups(uid_t uid);

/**
 * Do the above two steps, and setuid, and setgid...
 */
extern void sirius_set_credentials(uid_t uid);

_End_C_Header
