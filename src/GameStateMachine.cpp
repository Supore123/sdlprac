#include "GameStateMachine.h"
#include <iostream>

// -------------------------------------------------------------------------- //
//  Transition requests                                                        //
// -------------------------------------------------------------------------- //

void GameStateMachine::push(std::unique_ptr<IGameState> state)
{
    m_pending = { TransitionType::Push, std::move(state) };
}

void GameStateMachine::pop()
{
    m_pending = { TransitionType::Pop, nullptr };
}

void GameStateMachine::change(std::unique_ptr<IGameState> state)
{
    m_pending = { TransitionType::Change, std::move(state) };
}

// -------------------------------------------------------------------------- //
//  Per-frame driver                                                           //
// -------------------------------------------------------------------------- //

void GameStateMachine::processTransition()
{
    if (m_pending.type == TransitionType::None)
        return;

    switch (m_pending.type)
    {
        case TransitionType::Push:
        {
            if (!m_stack.empty())
                std::cout << "[GSM] Pausing state: " << m_stack.top()->getName() << "\n";

            m_stack.push(std::move(m_pending.state));
            m_stack.top()->onEnter();
            std::cout << "[GSM] Entered state: " << m_stack.top()->getName() << "\n";
            break;
        }

        case TransitionType::Pop:
        {
            if (!m_stack.empty())
            {
                std::cout << "[GSM] Exiting state: " << m_stack.top()->getName() << "\n";
                m_stack.top()->onExit();
                m_stack.pop();
            }

            if (!m_stack.empty())
                std::cout << "[GSM] Resuming state: " << m_stack.top()->getName() << "\n";
            break;
        }

        case TransitionType::Change:
        {
            if (!m_stack.empty())
            {
                std::cout << "[GSM] Exiting state: " << m_stack.top()->getName() << "\n";
                m_stack.top()->onExit();
                m_stack.pop();
            }

            m_stack.push(std::move(m_pending.state));
            m_stack.top()->onEnter();
            std::cout << "[GSM] Changed to state: " << m_stack.top()->getName() << "\n";
            break;
        }

        default:
            break;
    }

    m_pending = { TransitionType::None, nullptr };
}

void GameStateMachine::handleEvent(SDL_Event& e)
{
    if (!m_stack.empty())
        m_stack.top()->handleEvent(e);
}

void GameStateMachine::update(float dt)
{
    if (!m_stack.empty())
        m_stack.top()->update(dt);
}

void GameStateMachine::render()
{
    if (!m_stack.empty())
        m_stack.top()->render();
}

// -------------------------------------------------------------------------- //
//  Queries                                                                    //
// -------------------------------------------------------------------------- //

std::string GameStateMachine::currentName() const
{
    if (m_stack.empty())
        return "<empty>";
    return m_stack.top()->getName();
}
