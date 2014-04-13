/*
	Philip Romano
	4/8/2014
	gesturestategraph.cpp
*/

#include <string>
#include <map>
#include <iostream>

#include <boost/shared_ptr.hpp>

#include <Leap.h>

#include "gesturestategraph.h"
#include "gesturenode.h"

GestureStateGraph::GestureStateGraph() {

}

GestureStateGraph::~GestureStateGraph() {

}

bool GestureStateGraph::createNodeType(boost::shared_ptr<GestureNode> type) {
	if (type->getName().size() == 0)
		return false;

	std::pair< std::map<std::string,boost::shared_ptr<GestureNode> >::iterator,
	bool >
			result = mTypes.insert(
			std::pair<std::string,boost::shared_ptr<GestureNode> >
			(type->getName(), type));
	return result.second;
}

boost::shared_ptr<GestureNode> GestureStateGraph::removeNodeType(
		const std::string& name) {
	std::map<std::string,boost::shared_ptr<GestureNode> >::iterator it;
	it = mTypes.find(name);
	if (it == mTypes.end())
		return boost::shared_ptr<GestureNode>();
	else {
		boost::shared_ptr<GestureNode> value = it->second;
		mTypes.erase(it);
		return value;
	}
}

bool GestureStateGraph::addNode(const std::string& type,
		const std::string& nodeid) {
	if (!getType(type).get())
		return false;

	std::pair< std::map<std::string,NodeInstance >::iterator,bool >
			result = mNodes.insert(
			std::pair<std::string,NodeInstance>(
				nodeid, NodeInstance(type)));

	if (result.second) {
		// Initialize start and current nodes, if this is the first node added
		if (mStartNode.size() == 0) {
			mStartNode = nodeid;
			mCurrentNode = nodeid;
		}
		return true;
	} else
		return false;
}

bool GestureStateGraph::removeNode(const std::string& nodeid) {
	// TODO implement!
	return false;
}

bool GestureStateGraph::nodeExists(const std::string& nodeid) {
	return (mNodes.count(nodeid) > 0);
}

bool GestureStateGraph::setStart(const std::string& nodeid) {
	if (nodeExists(nodeid)) {
		mStartNode = nodeid;
		return true;
	} else
		return false;
}

boost::shared_ptr<GestureNode> GestureStateGraph::getStart() {
	return getTypeFromNode(mStartNode);
}

boost::shared_ptr<GestureNode> GestureStateGraph::getCurrentState() {
	return getTypeFromNode(mCurrentNode);
}

bool GestureStateGraph::addConnection(const std::string& start, int slot,
		const std::string& end) {
	if (nodeExists(start) && nodeExists(end)) {
		std::map<int,std::string>& slots = getAdjacencies(start);
		slots[slot] = end;
		return true;
	} else
		return false;
}

bool GestureStateGraph::removeConnection(const std::string& start, int slot) {
	if (nodeExists(start)) {
		std::map<int,std::string>& slots = getAdjacencies(start);
		return (slots.erase(slot) == 1);
	} else
		return false;
}

std::string GestureStateGraph::getSlot(const std::string& nodeid, int slot) {
	if (nodeExists(nodeid)) {
		std::map<int,std::string> adjacencies = getAdjacencies(nodeid);
		std::map<int,std::string>::iterator it = adjacencies.find(slot);
		if (it != adjacencies.end())
			return it->second;
		else
			return std::string();
	} else
		return std::string();
}

void GestureStateGraph::update(const Leap::Frame& frame) {
	std::string newCurrent = mCurrentNode;
	do {
		if (mCurrentNode.compare(newCurrent) != 0)
			setCurrent(newCurrent, frame);

		boost::shared_ptr<GestureNode> node = getTypeFromNode(mCurrentNode);
		if (node.get()) {
			int slot = node->evaluate(frame, mCurrentNode);
			newCurrent = getSlot(mCurrentNode, slot);
		} else
			newCurrent = "";
	} while (newCurrent.compare(mCurrentNode) != 0 && newCurrent.size() > 0);

	if (newCurrent.size() == 0)
		setCurrent(mStartNode, frame);
	else if (mCurrentNode.compare(newCurrent) != 0)
		setCurrent(newCurrent, frame);
}

boost::shared_ptr<GestureNode> GestureStateGraph::getType(
		const std::string& name) {
	std::map< std::string,boost::shared_ptr<GestureNode> >::iterator
		it = mTypes.find(name);
	if (it == mTypes.end())
		return boost::shared_ptr<GestureNode>();
	else
		return it->second;
}

boost::shared_ptr<GestureNode> GestureStateGraph::getTypeFromNode(
		const std::string& nodeid) {
	std::map<std::string,NodeInstance>::iterator it = mNodes.find(nodeid);
	if (it == mNodes.end())
		return boost::shared_ptr<GestureNode>();
	else
		return getType((it->second).type);
}

void GestureStateGraph::updateWithPrint(const Leap::Frame& frame) {
	std::string newCurrent = mCurrentNode;
	do {
		if (mCurrentNode.compare(newCurrent) != 0) {
			std::cout << newCurrent << std::endl;
			setCurrent(newCurrent, frame);
		}

		boost::shared_ptr<GestureNode> node = getTypeFromNode(mCurrentNode);
		if (node.get()) {
			int slot = node->evaluate(frame, mCurrentNode);
			newCurrent = getSlot(mCurrentNode, slot);
		} else
			newCurrent = "";
	} while (newCurrent.compare(mCurrentNode) != 0 && newCurrent.size() > 0);

	if (newCurrent.size() == 0)
		setCurrent(mStartNode, frame);
	else if (mCurrentNode.compare(newCurrent) != 0)
		setCurrent(newCurrent, frame);

	std::cout << "STATE: " << mCurrentNode << std::endl;
}


/*
   Private member functions
*/

std::map<int,std::string>& GestureStateGraph::getAdjacencies(
		const std::string& nodeid) {
	std::map<std::string,NodeInstance>::iterator it = mNodes.find(nodeid);
	if (it == mNodes.end())
		throw GestureStateException();
	else
		return (it->second).slots;
}

void GestureStateGraph::setCurrent(const std::string& nodeid,
		const Leap::Frame& frame) {
	boost::shared_ptr<GestureNode> previous = getTypeFromNode(mCurrentNode);
	boost::shared_ptr<GestureNode> next = getTypeFromNode(nodeid);
	previous->onLeave(frame, mCurrentNode);
	next->onEnter(frame, nodeid);
	mCurrentNode = nodeid;
}

