#include "addressbook.h"
//.clang-format file was not working as is with cLion due to "duplicate mapping key" errors.
// I have commented out the duplicate lines. Now clang-format seems to work.

// I had to change:
// #define calloc(num, size) my_calloc(nun, size, __LINE__) in memory_tracking.h to:
// #define calloc(num, size) my_calloc(num, size, __LINE__)
// to get calloc to work.

address_book_t* create_address_book() {
	address_book_t* addressBook = malloc(1 * sizeof(*addressBook));
	if (addressBook == NULL) {
		return NULL;
	}
	addressBook->head = NULL;

	return addressBook;
}

contact_t* create_contact(char* first_name, char* last_name, int age, char* email) {
	contact_t* new_contact = malloc(1 * sizeof(*new_contact));
	// contact_t* new_contact = calloc(1,  sizeof(*new_contact));
	// calloc (my_calloc) had problem with nun instead of num ...

	if (new_contact == NULL) {
		// maybe free created addressBook here | show error ?
		return NULL;
	}

	strcpy(new_contact->first_name, first_name);
	strcpy(new_contact->last_name, last_name);
	(new_contact->age) = age;
	strcpy(new_contact->email, email);

	return new_contact;
}

node_t* create_node() {
	node_t* new_node = malloc(1 * sizeof(*new_node));
	if (new_node == NULL) {
		return NULL;
	}
	return new_node;
}

void add_contact_helper(contact_t* contact, node_t* node) {
	node->data = contact;
	node->next = NULL;
	return;
}
//- adds a contact to the address book at the end of the list
void add_contact(address_book_t* address_book, contact_t* contact) {
	if (address_book == NULL || contact == NULL) {
		return;
	}

	node_t* new_node = create_node();
	if (new_node == NULL) {
		return;
	}
	add_contact_helper(contact, new_node);

	if (address_book->head == NULL) {
		address_book->head = new_node;
		return;
	} else {
		node_t* current_node = address_book->head;

		if (current_node->next == NULL) {
			current_node->next = new_node;
		} else {
			while (current_node->next != NULL) {
				current_node = current_node->next;
			}
			current_node->next = new_node;
		}
	}
}

node_t* find_node(address_book_t* address_book, char* first_name, char* last_name) {
	node_t* current_node = address_book->head;
	contact_t* current_contact = NULL;

	while (current_node->next != NULL) {
		// strcmp:
		current_contact = current_node->data;
		int strcmp_first_name = strcmp(current_contact->first_name, first_name);
		int strcmp_last_name = strcmp(current_contact->last_name, last_name);
		if (strcmp_first_name == 0 && strcmp_last_name == 0) {
			// found contact:
			return current_node;
		} else {
			current_node = current_node->next;
		}
	}
	// 1x after while for last node:
	// strcmp:
	current_contact = current_node->data;
	int strcmp_first_name = strcmp(current_contact->first_name, first_name);
	int strcmp_last_name = strcmp(current_contact->last_name, last_name);
	if (strcmp_first_name == 0 && strcmp_last_name == 0) {
		// found contact:
		return current_node;
	} else {
		// contact was not part of address_book:
		return NULL;
	}
}

contact_t* find_contact(address_book_t* address_book, char* first_name, char* last_name) {
	if (address_book == NULL) {
		return NULL;
	}
	if (address_book->head == NULL) {
		return NULL;
	}
	if ((first_name == NULL) && (last_name == NULL)) {
		return NULL;
	}
	node_t* contact_node = find_node(address_book, first_name, last_name);
	if (contact_node == NULL) {
		// contact not part of address_book:
		return NULL;
	}
	contact_t* contact = contact_node->data;
	return contact;
}

void remove_contact(address_book_t* address_book, contact_t* contact) {
	if ((address_book == NULL) || (address_book->head == NULL) || (contact == NULL)) {
		return;
	}
	// keep list structure in tact:
	node_t* contact_node = find_node(address_book, contact->first_name, contact->last_name);
	node_t* next_node = contact_node->next;

	node_t* before_node = address_book->head;
	while (before_node->next != contact_node) {
		before_node = before_node->next;
	}
	before_node->next = next_node;
	//

	// remove contact and node:
	free(contact_node->data);
	contact_node->data = NULL;
	free(contact_node);
	contact_node = NULL;
}

void print_address_book(address_book_t* address_book) {
	node_t* current = address_book->head;
	while (current != NULL) {
		contact_t* contact = (contact_t*)current->data;
		printf("%s %s, %d, %s\n", contact->first_name, contact->last_name, contact->age,
		       contact->email);
		current = current->next;
	}
}

contact_t* duplicate_contact(contact_t* contact) {
	// todo: unclear if supposed to be added to the address_book or not, check if error
	if (contact == NULL) {
		// contact not existent:
		return NULL;
	}
	contact_t* dup_contact =
	    create_contact(contact->first_name, contact->last_name, contact->age, contact->email);
	return dup_contact;
}

address_book_t* filter_address_book(address_book_t* address_book, bool (*filter)(contact_t*)) {
	if (address_book == NULL || address_book->head == NULL) {
		return NULL;
	}
	// creates a new address book containing only the contacts that satisfy the filter function
	// if no contact satisfys the filter -> empty address_book is created.
	address_book_t* new_address_book = create_address_book();

	// check if contact satisfies filter, for every contact in address_book:
	// use while-loop || for loop with contact count?
	node_t* curr_node = address_book->head;
	size_t contact_count = count_contacts(address_book);

	// TODO: CHECK if loop length is correct!!
	for (size_t runner = 1; runner <= contact_count; runner++) {
		contact_t* current_contact = curr_node->data;
		bool include = filter(current_contact);
		if (include == true) {
			// add contact to new_address_book (needs new node as well)
			contact_t* contact_to_add = duplicate_contact(current_contact);
			add_contact(new_address_book, contact_to_add);
		}
		curr_node = curr_node->next;
	}

	return new_address_book;
}

void swap(node_t* node_1, node_t* node_2) {

	void* n1_data = node_1->data;
	void* n2_data = node_2->data;

	(node_1->data) = n2_data;
	(node_2->data) = n1_data;
}

void sort_address_book(address_book_t* address_book, bool (*compare)(contact_t*, contact_t*)) {
	// using bubble-sort: idea and code-snippets from: https://www.geeksforgeeks.org/bubble-sort/
	if (address_book == NULL || address_book->head == NULL || address_book->head->next == NULL) {
		return;
	}

	node_t* node_1 = address_book->head;
	node_t* node_2 = node_1->next;

	size_t i, j;
	bool swapped;
	size_t n = count_contacts(address_book);

	for (i = 0; i < n - 1; i++) {

		swapped = false; // used for optimisation

		for (j = 0; j < (n - i - 1); j++) {
			contact_t* contact_1 = node_1->data;
			contact_t* contact_2 = node_2->data;

			bool comparison_val = compare(contact_1, contact_2);

			if (comparison_val == false) {
				// swap the data-pointers of the node:
				swap(node_1, node_2);

				swapped = true;
			}
			node_1 = node_1->next;
			node_2 = node_2->next;
		}
		node_1 = address_book->head;
		node_2 = node_1->next;

		// If no two elements were swapped by inner loop,
		// then break
		if (swapped == false) {
			break;
		}
	}
}

bool compare_by_name(contact_t* contact1, contact_t* contact2) {
	// TODO: unclear.
	//  returns ture when contact1 <= contact2:
	//  first looks at first_name;
	//  if equal -> looks at last_name
	//  if still equal -> is equal.
	if (contact1 == NULL || contact2 == NULL) {
		return NULL; // or return something else if contact non-existent?
	}

	// TODO: DEBUG:
	// fprintf(stderr, "first_name_ 1: %s\n\n", contact1->first_name);
	// fprintf(stderr, "first_name_ 2: %s\n\n", contact2->first_name);

	int strcmp_first_name = strcmp(contact1->first_name, contact2->first_name);

	if (strcmp_first_name < 0) {
		// TODO: DEBUG:
		// fprintf(stderr, "strcmp <= 0");
		return true;
	} else if (strcmp_first_name == 0) {
		int strcmp_last_name = strcmp(contact1->last_name, contact2->last_name);
		if (strcmp_last_name <= 0) {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

bool is_adult(contact_t* contact) {
	if (contact == NULL) {
		return NULL; // or return something else if contact non-existent?
	}
	if (contact->age >= 18) {
		return true;
	} else {
		return false;
	}
}

size_t count_contacts(address_book_t* address_book) {
	if (address_book == NULL) {
		return 0;
	}
	int counter = 0;
	node_t* current_node = address_book->head;
	while (current_node != NULL) {
		counter++;
		current_node = current_node->next;
	}
	return counter;
}

void free_address_book_helper(node_t* node) {
	if (node->next == NULL) {
		free(node->data);
		node->data = NULL;
		free(node);
		node = NULL;
		return;
	}
	free_address_book_helper(node->next);
	free(node->data);
	node->data = NULL;
	free(node);
	node = NULL;
	return;
}

void free_address_book(address_book_t* address_book) {
	if (address_book == NULL) {
		return;
	}
	if (address_book->head == NULL) {
		free(address_book);
		address_book = NULL;
		return;
	}

	node_t* head = address_book->head;
	free_address_book_helper(head);

	free(address_book);
	address_book = NULL;
	return;
}