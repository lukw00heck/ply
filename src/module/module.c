/*
 * Copyright 2015-2017 Tobias Waldekranz <tobias@waldekranz.com>
 *
 * This file is part of ply.
 *
 * ply is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, under the terms of version 2 of the
 * License.
 *
 * ply is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ply.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ply/module.h>
#include <ply/ply.h>

int default_loc_assign(node_t *call)
{
	node_t *probe = node_get_probe(call);
	node_t *varg;
	int reg;

	node_foreach(varg, call->call.vargs) {
		if (varg->type == TYPE_VAR ||
		    varg->dyn->loc == LOC_VIRTUAL)
			continue;

		switch (varg->dyn->type) {
		case TYPE_INT:
			reg = node_probe_reg_get(probe, 1);
			if (reg > 0) {
				varg->dyn->loc = LOC_REG;
				varg->dyn->reg = reg;
				continue;
			}
			/* no registers, fall-through and allocate on
			 * the stack */
		case TYPE_REC:
		case TYPE_STR:
			varg->dyn->loc  = LOC_STACK;
			varg->dyn->addr = node_probe_stack_get(probe, varg->dyn->size);
			continue;


		default:
			_e("argument %d of '%s' is of unknown type '%s'",
			   reg, call->string, type_str(varg->dyn->type));
			return -EINVAL;
		}
	}

	return 0;
}

int generic_get_func(const func_t **fs, node_t *call, const func_t **f)
{
	for (; *fs; fs++) {
		if (!strcmp((*fs)->name, call->string)) {
			*f = *fs;

			/* keep following alias links until we hit a
			 * concrete implementation */
			while ((*f)->alias)
				*f = (*f)->alias;
			return 0;
		}
	}

	return -ENOENT;
}

int module_get_func(const module_t *m, node_t *call, const func_t **f)
{
	if (call->call.module && strcmp(m->name, call->call.module))
		return -EINVAL;

	return m->get_func(m, call, f);
}

int modules_get_func(const module_t **ms, node_t *call, const func_t **f)
{
	int err = -EINVAL;
			
	for (; *ms; ms++) {
		if (call->call.module && strcmp((*ms)->name, call->call.module))
			continue;

		err = module_get_func(*ms, call, f);
		if (err != -ENOENT)
			break;
	}

	return err;
}
