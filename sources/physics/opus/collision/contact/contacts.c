/**
 * @file contact_manager.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/2/28
 *
 * @example
 *
 * @development_log
 *
 */

#include "data_structure/array.h"
#include "physics/opus/physics.h"
#include "physics/opus/physics_private.h"

opus_contact* opus_contact_create(opus_body* A, opus_body* B, opus_vec2 pa, opus_vec2 pb, opus_vec2 normal, opus_real depth)
{
	opus_contact* contact = calloc(1, sizeof(opus_contact));
	if (contact) {
		contact->A         = A;
		contact->B         = B;
		contact->pa        = pa;
		contact->pb        = pb;
		contact->depth     = depth;
		contact->normal    = normal;
		contact->tangent   = opus_vec2_perp(normal);
		contact->is_active = 1;
	}
	return contact;
}

void opus_contact_destroy(opus_contact* contact)
{
	free(contact);
}

char* opus_contacts_id(opus_body* A, opus_body* B)
{
	static char id[32];

	uint64_t id_min = A->id < B->id ? A->id : B->id;
	uint64_t id_max = A->id > B->id ? A->id : B->id;

	snprintf(id, 32, "%d,%d", id_min, id_max);
	return id;
}

opus_contacts* opus_contacts_create(opus_body* A, opus_body* B)
{
	opus_contacts* contacts = calloc(1, sizeof(opus_contacts));
	if (contacts) {
		contacts->A = A->id < B->id ? A : B;
		contacts->B = A->id > B->id ? A : B;
		strcpy(contacts->id, opus_contacts_id(A, B));
		opus_arr_create(contacts->contacts, sizeof(opus_contact *));
	}
	return contacts;
}

void opus_contacts_destroy(opus_contacts* contacts)
{
	opus_arr_destroy(contacts->contacts);
	free(contacts);
}
