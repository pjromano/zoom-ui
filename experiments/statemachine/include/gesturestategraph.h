/*
	Philip Romano
	4/8/2014
	gesturestategraph.h
*/

#ifndef GESTURESTATEGRAPH_H
#define GESTURESTATEGRAPH_H

#include <string>
#include <map>
#include <set>

#include <boost/shared_ptr.hpp>

#include <Leap.h>

#include "gesturenode.h"

/**
	A gesture state graph is a directed graph that is designed to allow
	explicit representation of a state machine, for the purposes of tracking
	the state of gestures.
	
	Each node in the graph has an indexed set of outgoing connections called
	branches. In addition, each node is associated with a specific GestureNode
	object, whose evaluate() function returns an index that corresponds to
	the branch that will be followed after evaluation.

	GestureNode itself is an abstract base class. GestureNode is designed to
	be derived in order to define the behavior of each node in the graph.

	Conceptually, nodes are divided into "state" nodes and "decision" nodes.

	A "state" node is one in which one of its branches leads back to itself.
	If a GestureNode returns a branch index that has not been connected with
	addConnection(), then the node defaults to connecting to itself.

	A "decision" node is one that will branch to a different node for
	all possible inputs.

	A gesture state graph MUST have at least one state node to work properly;
	otherwise, any attempt to advance the current state will continue
	indefinitely. Thus, when constructing the graph through GestureStateGraph,
	it is important to carefully design the graph to avoid cycles (though
	a typical graph for Gestures would have more states than decision nodes)
*/
class GestureStateGraph {
	public:
		/**
		  Create a new, empty gesture state graph.
		*/
		GestureStateGraph();

		/**
		  Destructor
		*/
		~GestureStateGraph();

		/**
		  Introduce a new type of GestureNode to this graph, so that nodes
		  of this type are known and can be added to the graph.

		    type : an instance of the desired GestureNode to be provided to
		           this graph

		  Returns true if this graph does not already have a type whose
		  getName() is equivalent to type.getName(), and the type was
		  successfully added. False otherwise.
		*/
		bool createNodeType(boost::shared_ptr<GestureNode> type);

		/**
		  Remove the GestureNode type whose getName() matches the given name.

		  Returns the GestureNode that was stored, or null if no such
		  GestureNode existed.
		*/
		boost::shared_ptr<GestureNode> removeNodeType(std::string name);

		/**
		  Add a node to the graph of the given type. The node can be later
		  identified with nodeid.

		  If this is the first node being added to the graph, then it becomes
		  the Starting node of the graph. (see setStart()/getStart()). It also
		  becomes the State Machine's current node.
		  
		  Returns true if the node was added; false otherwise (the provided
		  type name is not registered. See createNodeType())
		*/
		bool addNode(std::string type, std::string nodeid);

		/**
		  Remove the node with the given ID from the graph.

		  Returns true if a node was removed from the graph; false otherwise
		  (no node with the given ID existed)
		*/
		bool removeNode(std::string nodeid);

		/**
		  Returns true if a node with the given id currently exists in the
		  graph.
		*/
		bool nodeExists(std::string nodeid);

		bool setStart(std::string nodeid);

		boost::shared_ptr<GestureNode> getStart();

		boost::shared_ptr<GestureNode> getCurrentState();

		/**
		  Create a directed connection from the given slot in node start to
		  node end. If the slot was already connected to another node, then
		  it is updated to connect to end.

		  Returns true if a connection was created; false otherwise (start or
		  end is not an existent node)
		*/
		bool addConnection(std::string start, int slot, std::string end);

		/**
		  Remove a connection from the given slot in node start.

		  Returns true if a connection was removed; false otherwise (start is
		  not an existent node, or no connection existed in slot)
		*/
		bool removeConnection(std::string start, int slot);

		/**
		  Returns the name of the connected node in the slot number of the
		  given node. Returns an empty string if the slot is not connected or
		  if no node with the given name exists.
		*/
		std::string getSlot(std::string nodeid, int slot);

		/**
		  Clear all contents of the graph. Node Types are not removed.
		*/
		void clear();

		/**
		  Feeds the provided frame data to the current node and advances
		  the current node until a node advances to itself. This is considered
		  the current state of the gesture after processing one frame of data.

		  Consequently, a "state" node is implicitly a node in which one of its
		  connections leads back to itself. A state node is named such because
		  the "state" of the graph will remain at this node until one of the
		  exit conditions are met (a branch that connects elsewhere).

		  A "decision" node is one that will branch to a different node for
		  all possible inputs. As such, update() will never stop at a decision
		  node, and a decision node cannot be considered a state.

		  If the GestureNode returns a branch index that is not defined for
		  the current node, then the state machine defaults to resetting the
		  current node to the start node. This is to prevent unintended "dead
		  states", but could also be useful for defining the end of a gesture
		  without making explicit connections back to the "blank slate" start
		  state.
		*/
		void update(const Leap::Frame& frame);

		/**
		  Returns the GestureNode object associated with the given type name.
		*/
		boost::shared_ptr<GestureNode> getType(std::string type);

		/**
		  Returns the GestureNode object associated with the node with the
		  given id.
		*/
		boost::shared_ptr<GestureNode> getTypeFromNode(std::string nodeid);

	private:
		struct NodeInstance {
			std::string type;

			// The connections to other nodes (names), placed in the user-
			// specified slot number
			std::map<int,std::string> slots;

			// NodeInstance must be given a type name
			NodeInstance(std::string t) : type(t)
				{ }
		};

		// Associates GestureNode type name to a GestureNode object
		std::map< std::string,boost::shared_ptr<GestureNode> > mTypes;

		// Associates indiviudal node name to a NodeInstance, which holds
		// the type name and adjacencies (slots)
		std::map< std::string,NodeInstance > mNodes;

		// Adjacency list. map node name => set of names of adjacent nodes
//		std::map< std::string, std::set<std::string> > mAdjacencies;

		std::string mStartNode, mCurrentNode;


		/**
		  Returns a reference to the adjacency vector for the node with the
		  given name.

		  Throws GestureStateException if the given nodename does not exist
		  in the graph.
		*/
		std::map<int,std::string>& getAdjacencies(std::string nodeid);

};

class GestureStateException : public std::exception {
	public:
		const char* what() {
			return "Gesture state exception";
		}
};

#endif

