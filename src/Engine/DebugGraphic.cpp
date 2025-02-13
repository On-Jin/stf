#include "DebugGraphic.hpp"
#include <Engine/ShaderManager.hpp>
#include <Engine/Camera.hpp>
#include <iostream>
#include <algorithm>

DebugGraphic::DebugGraphic() :
		VAO(0),
		VBO(0),
		totalLine_(0),
		debugVertexBufferActualSize_(0),
		debugBuffer_(nullptr) {
    if (debug_)
	    std::cout << __FUNCTION_NAME__ << std::endl;
	buildBuffers_(debugVertexBufferActualSize_ + debugVertexBufferMarge_);
}

DebugGraphic::~DebugGraphic() {
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}

void DebugGraphic::buildBuffers_(unsigned int newSize) {
    if (debug_)
	    std::cout << __FUNCTION_NAME__ << std::endl;
	if (!VBO)
		glDeleteBuffers(1, &VBO);
	if (!VAO)
		glDeleteVertexArrays(1, &VAO);
	debugVertexBufferActualSize_ = newSize;

	if (debugBuffer_)
		delete [] debugBuffer_;
    if (debug_) {
        std::cout << "debugVertexBufferActualSize_ :["
                << debugVertexBufferActualSize_ << "]" << std::endl;
    }
		
	debugBuffer_ = new DebugVertex[debugVertexBufferActualSize_];


	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(DebugVertex) * debugVertexBufferActualSize_, nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), reinterpret_cast<const void *>(offsetof(DebugVertex, position)));
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), reinterpret_cast<const void *>(offsetof(DebugVertex, color)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void DebugGraphic::render() {
    if (debug_)
	    std::cout << __FUNCTION_NAME__ << std::endl;
	ShaderManager::Get().getShader("debugWireFrame").activate();
	ShaderManager::Get().getShader("debugWireFrame").setMat4("projection", Camera::focus->getProjectionMatrix());
	ShaderManager::Get().getShader("debugWireFrame").setMat4("view", Camera::focus->getViewMatrix());

	glBindVertexArray(VAO);
	glLineWidth(2.0f);
	glDrawArrays(GL_LINES, 0, totalLine_);
	glBindVertexArray(0);
}

 void DebugGraphic::addDebugObject(ADebugObject &debugObject) {
    if (debug_)
        std::cout << __FUNCTION_NAME__ << std::endl;
	bufferObject_.push_back(&debugObject);
    //updateInfo_();
}

void DebugGraphic::removeDebugObject(ADebugObject &debugObject) {
    if (debug_)
        std::cout << __FUNCTION_NAME__ << std::endl;
    auto it = find(bufferObject_.begin(),bufferObject_.end(), &debugObject);

	if (it != bufferObject_.end()) {
        bufferObject_.erase(it);
    }
	updateInfo_();
}


void DebugGraphic::updateInfo_() {
    if (debug_)
	    std::cout << __FUNCTION_NAME__ << std::endl;
	totalLine_ = 0;
	for (ADebugObject *debugObject : bufferObject_) {
        totalLine_ += debugObject->linesObject_.size();
	}
	if (totalLine_ > debugVertexBufferActualSize_) {
        if (totalLine_ > debugVertexBufferActualSize_)
            totalLine_ += debugVertexBufferMarge_;
        buildBuffers_(totalLine_);
	}
	updateDebugBuffer_();
}

void DebugGraphic::updateDebugBuffer_() {
	std::cout << __FUNCTION_NAME__ << std::endl;
	unsigned int index = 0;
	for (ADebugObject *debugObject : bufferObject_) {
        if (debug_)
            std::cout << "debugVertexBufferActualSize_ [" << debugVertexBufferActualSize_ << "] | Index [" << index << "] | [" << debugObject->linesObject_.size() << "]" << std::endl;

		memcpy(reinterpret_cast<void *>(&debugBuffer_[index]),
			   reinterpret_cast<void *>(&debugObject->linesObject_.front()),
			   sizeof(DebugVertex) * debugObject->linesObject_.size());
		index += debugObject->linesObject_.size();
	}
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DebugVertex) * totalLine_, debugBuffer_);
}

void DebugGraphic::updateDebugObject_(ADebugObject &object) {
    if (debug_)
        std::cout << __FUNCTION_NAME__ << std::endl;
    unsigned int index = 0;
    for (ADebugObject *debugObject : bufferObject_) {
        if (&object == debugObject) {
            memcpy(reinterpret_cast<void *>(&debugBuffer_[index]),
                   reinterpret_cast<void *>(&debugObject->linesObject_.front()),
                   sizeof(DebugVertex) * debugObject->linesObject_.size());

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, sizeof(DebugVertex) * index, sizeof(DebugVertex) * debugObject->linesObject_.size(), reinterpret_cast<void *>(&debugObject->linesObject_.front()));
        }
        index += debugObject->linesObject_.size();
    }
}

bool DebugGraphic::debug_ = false;