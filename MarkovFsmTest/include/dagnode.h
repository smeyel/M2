#ifndef __DAGNODE_H
#define __DAGNODE_H

#include <vector>

using namespace std;

namespace smeyel
{
	class dagnode
	{
	private:
		static int nextFreeNodeID;
	protected:
		int nodeID;
		vector<dagnode *> parents;	// may contain NULL-s
		vector<dagnode *> children;	// may contain NULL-s

		// --- parent handling
		int getParentIdx(dagnode *parent)
		{
			for(int i=0; i<parents.size(); i++)
			{
				if (parents[i]==parent)
				{
					return i;
				}
			}
			return -1;
		}

		int status;	// Auxiliary status value used by the applications

	public:
		dagnode()
		{
			nodeID=nextFreeNodeID++;
		}

		// --- parent handling
		/** Adds given node to the parent list, if not yet present. */
		void addParent(dagnode *parent)	// called by setChild
		{
			int idx = getParentIdx(parent);
			if (idx==-1)
			{
				parents.push_back(parent);
			}
		}

		/** Remove node from list of parents, if present.
			Typically used by setChild().
		*/
		void removeParent(dagnode *parent)	// called by setChild
		{
			int idx = getParentIdx(parent);
			if (idx==-1)
			{
				parents[idx]=NULL;
			}
		}

		/** Returns parent pointer of given index. Used for enumerations.
			TODO: make iterator instead!
		*/
		dagnode *getParent(unsigned int idx)
		{
			if (idx < parents.size())
			{
				return parents[idx];
			}
			return NULL;
		}

		int getParentNum()
		{
			return parents.size();
		}

		// --- child handling
		/** Sets the child for given idx.
			Fixes parent pointers of old and new children.
			@param idx	Idx of child to set.
			@param child	Pointer to new child node. May be NULL.
		*/
		void setChild(unsigned idx, dagnode *child)
		{
			if (idx >= children.size())
			{
				children.reserve(idx+1);	// By default NULL lesz?
			}
			// Had a previous value?
			dagnode *oldChild = children[idx];
			if (oldChild!=NULL)
			{
				oldChild->removeParent(this);
			}
			children[idx] = child;
			if (child != NULL)
				child->addParent(this);
		}

		/** Get the child corresponding to idx. May be asked to create on demand. Otherwise, NULL may be retured. */
		dagnode *getChild(unsigned int idx, bool createOnDemand=false)
		{
			dagnode *result = NULL;
			if (idx < children.size())
			{
				result = children[idx];
			}
			if (result==NULL && createOnDemand)
			{
				result = new dagnode();
				setChild(idx,result);
			}
			return result;
		}

		/** Size of children array.
			@warning Children array may contain NULL-s!
		*/
		int getChildNum()
		{
			return children.size();
		}

		// --- status handling
		int setStatus(int newStatus)
		{
			status = newStatus;
		}

		int getStatus()
		{
			return status;
		}

		// --- advanced functions: rerouting of pointers
		/** Assumed to be started from the only root, so parents are always already fixed.
			@warning Always start from the only root node.
			@warning Not suitable to replace the root node.
		*/
		void replaceNodeForwardRecursive(dagnode *oldNode, dagnode *newNode)
		{
			assert(oldNode != NULL);
			assert(newNode != NULL);
			for(int i=0; i<getChildNum(); i++)
			{
				dagnode *child = getChild(i,false);
				if (child == oldNode)
				{
					setChild(i,newNode);	// fixes parent pointers to this node
				}
			}

			// recursive call
			for(int i=0; i<getChildNum(); i++)
			{
				dagnode *child = getChild(i,false);
				if (child != NULL)
				{
					replaceNodeForwardRecursive(oldNode,newNode);
				}
			}
		}

		// TODO: Forward recursive check subgraph status homogenity

		// TODO: Cut subgraph if homogeneous and has same status. (Forward recursive if inhomogeneous.)

		// TODO: Check forward subgraph equivalence (check parents coming from outside!)
	};

	int dagnode::nextFreeNodeID = 0;
}

#endif